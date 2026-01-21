
#include <ctime>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../../include/server.hpp"
#include "../../include/router.hpp"
#include "../../include/requestHandler.hpp"
#include "../../include/httpResponse.hpp"

Server::Server(ConfigParser &config): config(config), poller(event::make_poller()) {}

Server::~Server() {}

void Server::init()
{
    const std::vector<ServerConfig>& servers = config.getServers();

    for (const auto& server : servers)
    {
        for (const auto& lp : server.listen_port)
        {
            int fd = create_and_listen(lp);
            listen_sockets.emplace_back(fd, lp.ip, lp.port);
            listen_fd_set.insert(fd);
            
            std::cout << "\nListening on http://"
                        << inet_ntoa(*(struct in_addr*)&lp.ip)
                        << ":" << lp.port << "\n";
        }
    }
}

void Server::run()
{

    while (true)
    {
        auto events = poller->wait(1000);
    
        for (const auto& event : events)
        {
            if (listen_fd_set.count(event.fd) > 0)
                new_connection(event.fd);
            if (event.readable)
                read_client(event.fd);
            if (event.writable)
                client_write(event.fd);
        } 
        check_timeouts(); 
    }
}

void client_write()
{

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
        
        // Store client with connection info
        //check dıf wıth requests
        clients.emplace(client_fd, ClientCon(client_fd, ls->ip, ls->port, time(nullptr)));
        
        std::cout << "\nNew client " << client_fd << " connected to " << ls->port << "\n";
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
    
    if (bytes <= 0)
    {
        close_client(client_fd);
        return;
    }
    
    ParseState state;
    state = conn.req.parseRequest(buffer.data(), bytes);
    if (state == COMPLETE) 
    {
        conn.req.printRequest();
        std::string host = conn.req.getHeaderVal("Host");
        conn.servConf = findServer(conn.ip, conn.port, host);
        
        if (!conn.servConf)
        {
            send_error_response(client_fd, 404, "miao");
            conn.req = HttpRequest();
            close_client(client_fd);
            return;
        }
        process_request(conn);
    }
    else if (state == ERROR)
    {
        parsedRequest reqi = conn.req.getRequest();
        std::cout << "\nerror: " << reqi.error_info << std::endl;
        conn.req = HttpRequest();
        send_error_response(client_fd, reqi.error_code, reqi.error_info);
        close_client(client_fd);
    }
}

const ServerConfig* Server::findServer(uint32_t ip, uint16_t port, const std::string& host_header)
{
	const std::vector<ServerConfig>& servers = config.getServers();

    for (const auto& server : servers)
  	{
    	for (const auto& lp : server.listen_port)
    	{
      		if (lp.ip == ip && lp.port == port)
      		{
        		if (!host_header.empty())
        		{
          			for (const auto& name : server.server_name)
          			{
            			if (name == host_header || name == "_")
              				return &server;
          			}
        		}
        		else
          			return &server;
      		}
    	}
  	}
  	for (const auto& server : servers)
  	{
    	for (const auto& lp : server.listen_port)
    	{
      		if (lp.port == port)
        	return &server;
    	}
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

    bool keep_alive;
    std::string connection = conn.req.getHeaderVal("Connection");
    if (req.http_v == "HTTP/1.1")
        keep_alive = (connection != "close");
    else if (req.http_v == "HTTP/1.0")
        keep_alive = (connection == "keep-alive");
    //create response object(route, servConf, req)
    RequestHandler handler(routy, req, *conn.servConf, keep_alive);
    HttpResponse res = handler.handle();
    res.send_response(conn.fd);

    /* send_response(conn.fd, keep_alive); */
    if (!keep_alive)
    {
        conn.req = HttpRequest();
        close_client(conn.fd);
    }
    else
    {
        conn.req.clear();
        conn.last_act = std::time(nullptr);
    }
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

void Server::send_response(int client_fd, bool keep_alive)
{
    const std::string body = "Miau Miauu Miauuuu!!!!!\n";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: " + std::string(keep_alive ? "keep-alive" : "close") + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;

    const char *data = response.data();
    int remaining = static_cast<int>(response.size());

    while (remaining > 0)
    {
      int written = event::send_data(client_fd, data, remaining);
      if (written <= 0)
        break;
      data += written;
      remaining -= written;
    }
}
    
void Server::send_error_response(int client_fd, int status_code, std::string error_info)
{
    // server_config->error_pages
    std::string response = "HTTP/1.1 " + std::to_string(status_code) + " Error\r\nConnection: close\r\ninfo: " + error_info + "\r\n\r\n";
    event::send_data(client_fd, response.data(), response.size());
}

void Server::close_client(int client_fd)
{
    poller->remove(client_fd);
    clients.erase(client_fd);
    event::close_socket(client_fd);
    std::cout << "\nClient " << client_fd << " disconnected\n";
}
    
void Server::check_timeouts()
{
    time_t now = std::time(nullptr);
    std::vector<int> to_close;
    
    for (auto& [fd, conn] : clients)
    {
        int timeout;
        if (conn.servConf && conn.servConf->timeout > 0)
            timeout = conn.servConf->timeout;
        else
            timeout = 60;

        if (now - conn.last_act > timeout)
        {
            std::cout << "\nClient " << fd << " timed out\n";
            conn.req = HttpRequest();
            to_close.push_back(fd);
        }
    }
    
    for (int fd : to_close)
        close_client(fd);
}