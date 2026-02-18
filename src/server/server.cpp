
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cerrno>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include "../../include/server.hpp"
#include "../../include/router.hpp"
#include "../../include/requestHandler.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/utils.hpp"

namespace
{
int map_static_open_errno_to_status(int err)
{
    if (err == ENOENT || err == ENOTDIR)
        return NotFound;
    if (err == EACCES)
        return Forbidden;
    return InternalServerError;
}

std::string static_content_type(const std::string& file_path)
{
    std::string content_type = getMime(file_path);
    if (content_type.find("text/") == 0 || content_type == "application/javascript")
        content_type += "; charset=utf-8";
    return content_type;
}
}

Server* Server::active_instance = nullptr;

Server::Server(ConfigParser &config):
    config(config),
    poller(event::make_poller()),
    cgi_manager(nullptr),
    signal_pipe{ -1, -1 },
    signal_pipe_initialized(false)
{
    cgi_manager = std::make_unique<CgiManager>(*poller);
    active_instance = this;
}

Server::~Server()
{
    if (cgi_manager)
        cgi_manager->shutdown();
    if (signal_pipe_initialized)
    {
        poller->remove(signal_pipe[0]);
        close(signal_pipe[0]);
        close(signal_pipe[1]);
        signal_pipe_initialized = false;
        struct sigaction sa{};
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sigaction(SIGCHLD, &sa, nullptr);
    }
    if (active_instance == this)
        active_instance = nullptr;
}

void Server::init()
{
    const std::vector<ServerConfig>& servers = config.getServers();

    for (const auto& server : servers)
    {
        for (const auto& lp : server.listen_port)
        {
            int fd = -1;
            for (const auto& ls : listen_sockets)
            {
                if (ls.ip == lp.ip && ls.port == lp.port)
                {
                    fd = ls.fd;
                    break;
                }
            }
            if (fd < 0)
            {
                fd = create_and_listen(lp);
                if (fd < 0)
                    throw std::runtime_error("Failed to bind socket");
                listen_sockets.emplace_back(fd, lp.ip, lp.port);
                #ifdef DEBUG
                std::cout << "\nListening on http://"
                        << inet_ntoa(*(struct in_addr*)&lp.ip)
                        << ":" << lp.port << "\n";
                #endif
            }
            listen_fd_set.insert(fd);
        }
    }
    setup_signal_pipe();
}

void Server::run()
{

    while (true)
    {
        auto events = poller->wait(1000);
    
        for (const auto& event : events)
        {
            if (signal_pipe_initialized && event.fd == signal_pipe[0])
            {
                handle_signal_pipe();
                continue;
            }
            if (dispatch_cgi_event(event))
                continue;
            if (listen_fd_set.count(event.fd) > 0)
            {
                new_connection(event.fd);
                continue;
            }
            if (event.readable)
                read_client(event.fd);
            if (event.writable)
                client_write(event.fd);
        } 
        check_timeouts(); 
    }
}

//update(fd, read, write) -> event::update(fd, true, true);
void Server::client_write(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;

    ClientCon& conn = it->second;

    if (!conn.res_ready && !conn.file_stream_active)
        return;

    if (!conn.response_out.empty())
    {
        if (conn.sent >= conn.response_out.size())
        {
            conn.response_out.clear();
            conn.sent = 0;
        }
        else
        {
            size_t remaining = conn.response_out.size() - conn.sent;
            int to_send = static_cast<int>(
                remaining > static_cast<size_t>(std::numeric_limits<int>::max())
                    ? std::numeric_limits<int>::max()
                    : remaining);
            int written = event::send_data(client_fd, conn.response_out.data() + conn.sent, to_send);

            if (written > 0)
            {
#ifdef DEBUG
                std::cout << "[WRITE] fd=" << conn.fd
                        << " wrote=" << written
                        << " sent=" << conn.sent + static_cast<size_t>(written)
                        << " total=" << conn.response_out.size()
                        << " cgi=" << conn.cgi_active
                        << " file_stream=" << conn.file_stream_active
                        << " ka=" << conn.keep_alive
                        << " close_after=" << conn.close_after_send
                        << "\n";
#endif
                conn.sent += static_cast<size_t>(written);
                conn.last_act = std::time(nullptr);
                drain_client_backpressure(conn);

                if (conn.sent == conn.response_out.size())
                {
                    conn.response_out.clear();
                    conn.sent = 0;
                    if (!conn.file_stream_active)
                        finalize_response_write(conn);
                }
                return;
            }
            if (written == 0)
            {
                return;
            }
            close_client(conn.fd);
            return;
        }
    }

    if (conn.file_stream_active)
    {
        write_static_file_chunk(conn);
        return;
    }

    if (conn.res_ready)
        finalize_response_write(conn);
}

bool Server::start_static_file_stream(ClientCon& conn, const Route& route, const parsedRequest& req)
{
    reset_static_file_stream(conn);
    conn.response_out.clear();
    conn.sent = 0;
    conn.res_ready = false;
    conn.close_after_send = false;

    auto queue_error = [&](int status)
    {
        HttpResponse res(conn.keep_alive ? "keep-alive" : "close", req.http_v);
        std::string body = generateErrorPage(status, mapStatus(status));
        res.setStatus(status);
        res.setHeader("Content-Type", "text/html");
        res.setHeader("Content-Length", std::to_string(body.size()));
        res.setBody(body);
        conn.response_out = res.buildResponse();
        conn.sent = 0;
        conn.res_ready = true;
    };

    int open_flags = O_RDONLY;
#ifdef O_CLOEXEC
    open_flags |= O_CLOEXEC;
#endif
    int fd = open(route.file_path.c_str(), open_flags);
    if (fd < 0)
    {
        queue_error(map_static_open_errno_to_status(errno));
        return false;
    }

    struct stat st{};
    if (fstat(fd, &st) != 0)
    {
        int status = (errno == EACCES) ? Forbidden : InternalServerError;
        close(fd);
        queue_error(status);
        return false;
    }
    if (!S_ISREG(st.st_mode))
    {
        close(fd);
        queue_error(Forbidden);
        return false;
    }
    if (st.st_size < 0)
    {
        close(fd);
        queue_error(InternalServerError);
        return false;
    }

    unsigned long long file_size = static_cast<unsigned long long>(st.st_size);
    if (file_size > static_cast<unsigned long long>(std::numeric_limits<size_t>::max()))
    {
        close(fd);
        queue_error(InternalServerError);
        return false;
    }

    HttpResponse res(conn.keep_alive ? "keep-alive" : "close", req.http_v);
    res.setStatus(Ok);
    res.setHeader("Content-Type", static_content_type(route.file_path));
    res.setHeader("Content-Length", std::to_string(file_size));

    conn.response_out = res.buildResponse();
    conn.sent = 0;
    conn.res_ready = true;
    conn.file_fd = fd;
    conn.file_stream_active = true;
    conn.file_bytes_remaining = static_cast<size_t>(file_size);
    conn.file_buf_len = 0;
    conn.file_buf_off = 0;
    return true;
}

bool Server::write_static_file_chunk(ClientCon& conn)
{
    if (!conn.file_stream_active)
        return finalize_response_write(conn);

    if (conn.file_buf_off >= conn.file_buf_len)
    {
        conn.file_buf_off = 0;
        conn.file_buf_len = 0;

        if (conn.file_bytes_remaining == 0)
        {
            reset_static_file_stream(conn);
            return finalize_response_write(conn);
        }

        size_t to_read = conn.file_buf.size();
        if (to_read > conn.file_bytes_remaining)
            to_read = conn.file_bytes_remaining;

        ssize_t read_bytes;
        do
        {
            read_bytes = read(conn.file_fd, conn.file_buf.data(), to_read);
        } while (read_bytes < 0 && errno == EINTR);

        if (read_bytes < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true;
            close_client(conn.fd);
            return false;
        }
        if (read_bytes == 0)
        {
            // Header is already out; avoid mismatched content-length by closing hard.
            close_client(conn.fd);
            return false;
        }

        conn.file_buf_len = static_cast<size_t>(read_bytes);
        conn.file_bytes_remaining -= conn.file_buf_len;
    }

    size_t pending = conn.file_buf_len - conn.file_buf_off;
    int written = event::send_data(conn.fd, conn.file_buf.data() + conn.file_buf_off, static_cast<int>(pending));
    if (written > 0)
    {
        conn.file_buf_off += static_cast<size_t>(written);
        conn.last_act = std::time(nullptr);
        drain_client_backpressure(conn);
        if (conn.file_buf_off == conn.file_buf_len && conn.file_bytes_remaining == 0)
        {
            reset_static_file_stream(conn);
            return finalize_response_write(conn);
        }
        return true;
    }
    if (written == 0)
        return true;
    close_client(conn.fd);
    return false;
}

void Server::reset_static_file_stream(ClientCon& conn)
{
    if (conn.file_fd >= 0)
        close(conn.file_fd);
    conn.file_fd = -1;
    conn.file_stream_active = false;
    conn.file_bytes_remaining = 0;
    conn.file_buf_len = 0;
    conn.file_buf_off = 0;
}

bool Server::finalize_response_write(ClientCon& conn)
{
    conn.res_ready = false;

    if (conn.cgi_active)
    {
#ifdef DEBUG
        std::cout << "[WRITE] fd=" << conn.fd
                  << " CGI response complete"
                  << " close_after=" << conn.close_after_send
                  << " force_close=" << conn.cgi_force_close
                  << " ka=" << conn.keep_alive << "\n";
#endif
        conn.cgi_active = false;
        if (conn.close_after_send || conn.cgi_force_close || !conn.keep_alive)
        {
            conn.close_after_send = false;
            close_client(conn.fd);
            return false;
        }
        poller->update(conn.fd, true, false);
        conn.last_act = std::time(nullptr);
        return true;
    }

    if (conn.close_after_send || !conn.keep_alive)
    {
        conn.close_after_send = false;
        close_client(conn.fd);
        return false;
    }
    conn.close_after_send = false;
    conn.req.clear();
    conn.last_act = std::time(nullptr);
    poller->update(conn.fd, true, false);
    return true;
}

void Server::new_connection(int listen_fd)
{
    while (true)
    {
        int client_fd = event::accept_connection(listen_fd);
        if (client_fd == -1)
            break;  // No more connections
        
        // Find which IP/port this listen socket is on
        const ListenSocket* ls = get_listen_socket(listen_fd);
        if (!ls)
        {
            event::close_socket(client_fd);
            continue;
        }
        
        event::set_non_blocking(client_fd);
        poller->add(client_fd, true, false);
        
        auto inserted = clients.emplace(client_fd, ClientCon(client_fd, ls->ip, ls->port));
        if (inserted.second)
            capture_peername(client_fd, inserted.first->second);
        #ifdef DEBUG
        std::cout << "\nNew client " << client_fd << " connected to " << ls->port << "\n";
        #endif
    }
}

void Server::read_client(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;
    
    ClientCon& conn = it->second;
    conn.last_act = std::time(nullptr);
    
    // Read from client
    std::array<char, 4096> buffer{};
    int bytes = event::receive_data(client_fd, buffer.data(), buffer.size());

#ifdef DEBUG
    if (bytes > 0)
    {
        std::string preview;
        preview.reserve(static_cast<size_t>(bytes));
        for (int i = 0; i < bytes; ++i)
        {
            char ch = buffer[static_cast<size_t>(i)];
            if (ch == '\r')
                preview += "\\r";
            else if (ch == '\n')
                preview += "\\n";
            else if (static_cast<unsigned char>(ch) < 0x20)
                preview += '.';
            else
                preview += ch;
        }
        std::cout << "[READ] fd=" << conn.fd
                  << " bytes=" << bytes
                  << " data=\"" << preview << "\"\n";
    }
#endif

    if (bytes <= 0)
    {
#ifdef DEBUG
        std::cout << "[READ] fd=" << conn.fd
                  << " recv=" << bytes
                  << " -> closing\n";
#endif
        close_client(client_fd);
        return;
    }

    while (true)
    {
        ParseState state = conn.req.parseRequest(buffer.data(), bytes);
        bytes = 0;
#ifdef DEBUG
        std::cout << "[READ] fd=" << conn.fd
                  << " state=" << state
                  << " uri=" << conn.req.getUri()
                  << "\n";
#endif
        
        if (state == COMPLETE)
        {
            #ifdef DEBUG
            conn.req.printRequest();
            #endif

            std::string host = conn.req.getHeaderVal("host");
            conn.servConf = findServer(conn.ip, conn.port, host);
            if (!conn.servConf)
            {
                fallback_error(conn, 500);
                return;
            }
            process_request(conn);
            conn.req.clear();
            continue;
        }
        if (state == ERROR)
        {
            int status = conn.req.getRequest().error_code;
            if (status <= 0)
                status = 400;
            fallback_error(conn, status);
            return;
        }
        break;
    }
}

void Server::fallback_error(ClientCon& conn, int status)
{
    std::string body = generateErrorPage(status, mapStatus(status));

    conn.response_out =
        "HTTP/1.1 " + std::to_string(status) + " " + mapStatus(status) + "\r\n"
        "Date: " + httpDate() + "\r\n"
        "Server: CatSurf\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    conn.keep_alive = false;
    conn.res_ready = true;
    poller->update(conn.fd, false, true);
}


const ServerConfig* Server::findServer(uint32_t ip, uint16_t port, const std::string& host_header)
{
	const std::vector<ServerConfig>& servers = config.getServers();
    const ServerConfig *firstMatch = nullptr;

    for (const auto& server : servers)
  	{
        for (const auto& lp : server.listen_port)
    	{
      		if (lp.ip == ip && lp.port == port)
      		{
        		if (!firstMatch)
                    firstMatch = &server; 
                if (!host_header.empty())
        		{
          			for (const auto& name : server.server_name)
          			{
                        if (name == str_tolower(host_header) || name == "_")
                            return &server;
          			}
        		}
      		}
    	}
  	}
    if (firstMatch)
    {
        return firstMatch;
    }
  	return nullptr;
}

const ListenSocket* Server::get_listen_socket(int fd) const
{
    for (const auto& ls : listen_sockets)
    {
        if (ls.fd == fd)
            return &ls;
    }
    return nullptr;
}

void Server::process_request(ClientCon& conn)
{
    Router r(*conn.servConf, conn.req.getRequest());
    Route routy = r.route();
    parsedRequest req = conn.req.getRequest();

    std::string connection = str_tolower(conn.req.getHeaderVal("connection"));
    if (req.http_v == "HTTP/1.1")
        conn.keep_alive = connection != "close";
    else if (req.http_v == "HTTP/1.0")
        conn.keep_alive = connection == "keep-alive";

    if (routy.type == FILES && req.method == "GET")
    {
        start_static_file_stream(conn, routy, req);
        poller->update(conn.fd, false, true);
        conn.last_act = std::time(nullptr);
        return;
    }

    if (routy.type == CGI)
    {
#ifdef DEBUG
        std::cout << "[ROUTE] CGI uri=" << req.uri
                  << " query=" << req.query
                  << " script=" << routy.script_path
                  << " path_info=" << routy.path_info << "\n";
#endif
        if (!cgi_manager || !cgi_manager->launch(routy, req, conn, *conn.servConf))
        {
#ifdef DEBUG
            std::cout << "[ROUTE] CGI launch failed" << "\n";
#endif
            fallback_error(conn, BadGateway);
            conn.keep_alive = false;
            return;
        }
        conn.last_act = std::time(nullptr);
        return;
    }

#ifdef DEBUG
    std::cout << "[ROUTE] type=" << static_cast<int>(routy.type)
              << " uri=" << req.uri
              << " file=" << routy.file_path << "\n";
#endif

    RequestHandler handler(routy, req, *conn.servConf, conn.keep_alive);
    HttpResponse res = handler.handle();
    conn.response_out = res.buildResponse();
    conn.res_ready = true;

    poller->update(conn.fd, false, true);
    conn.last_act = std::time(nullptr);
}

int Server::create_and_listen(const ListenPort& lp)
{
    int fd = event::create_socket();
    event::set_socket_reuse(fd);
    bind_and_listen(fd, lp.port, lp.ip);
    event::set_non_blocking(fd);
    poller->add(fd, true, false);
    return fd;
}

void Server::bind_and_listen(int fd, uint16_t port, uint32_t ip)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
      throw std::runtime_error("bind failed");
    }

    if (listen(fd, SOMAXCONN) != 0)
    {
      throw std::runtime_error("listen failed");
    }
}

void Server::setup_signal_pipe()
{
    if (signal_pipe_initialized)
        return;

    if (pipe(signal_pipe) != 0)
        throw std::runtime_error("Failed to create signal pipe");

    for (int i = 0; i < 2; ++i)
    {
        int flags = fcntl(signal_pipe[i], F_GETFL, 0);
        if (flags == -1 || fcntl(signal_pipe[i], F_SETFL, flags | O_NONBLOCK) == -1)
        {
            close(signal_pipe[0]);
            close(signal_pipe[1]);
            throw std::runtime_error("Failed to configure signal pipe");
        }
    }

    poller->add(signal_pipe[0], true, false);
    signal_pipe_initialized = true;

    struct sigaction sa{};
    sa.sa_handler = Server::sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, nullptr) != 0)
    {
        poller->remove(signal_pipe[0]);
        close(signal_pipe[0]);
        close(signal_pipe[1]);
        signal_pipe_initialized = false;
        throw std::runtime_error("Failed to install SIGCHLD handler");
    }
}

void Server::sigchld_handler(int)
{
    if (!active_instance || !active_instance->signal_pipe_initialized)
        return;
    char byte = 1;
    ssize_t r = write(active_instance->signal_pipe[1], &byte, 1);
    (void)r;
}

void Server::handle_signal_pipe()
{
    if (!signal_pipe_initialized)
        return;

    char buffer[64];
    while (true)
    {
        ssize_t n = read(signal_pipe[0], buffer, sizeof(buffer));
        if (n <= 0)
            break;
    }
    reap_children();
}

void Server::reap_children()
{
    if (!cgi_manager)
        return;

    int status = 0;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        cgi_manager->handleChildExit(pid, status);
}

bool Server::dispatch_cgi_event(const event::PollEvent& ev)
{
    if (cgi_manager && cgi_manager->handlesFd(ev.fd))
    {
        cgi_manager->handleEvent(ev.fd, ev.readable, ev.writable);
        return true;
    }
    return false;
}

void Server::drain_client_backpressure(ClientCon& conn)
{
    if (cgi_manager && conn.cgi_active)
        cgi_manager->notifyClientWritable(conn.fd);
}

void Server::capture_peername(int client_fd, ClientCon& conn)
{
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(client_fd, reinterpret_cast<sockaddr*>(&addr), &len) == 0)
    {
        char buf[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf)))
            conn.remote_addr = buf;
        conn.remote_port_net = addr.sin_port;
    }
}

void Server::check_timeouts()
{
    time_t now = std::time(nullptr);
    std::vector<int> to_close;
    
    for (auto& [fd, conn] : clients)
    {
        int timeout = 60;
        if (conn.servConf && conn.servConf->timeout > 0)
            timeout = conn.servConf->timeout;

        if (now - conn.last_act > timeout)
        {
            #ifdef DEBUG
            std::cout << "\nClient " << fd << " timed out\n";
            #endif
            conn.req = HttpRequest();
            to_close.push_back(fd);
        }
    }
    
    for (int fd : to_close)
        close_client(fd);

    if (cgi_manager)
        cgi_manager->checkTimeouts(now);
}

void Server::close_client(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;

    reset_static_file_stream(it->second);

    if (cgi_manager)
        cgi_manager->handleClientClose(client_fd);
    poller->remove(client_fd);
    clients.erase(it);
    event::close_socket(client_fd);
    #ifdef DEBUG
    std::cout << "\nClient " << client_fd << " disconnected\n";
    #endif
}
