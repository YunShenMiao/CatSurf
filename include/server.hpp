#ifndef SERVER_HPP
#define SERVER_HPP

#include <array>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <memory>

#include "cgi.hpp"
#include "configParser.hpp"
#include "poller.h"
#include "httpRequest.hpp"

/* IPv4 = AF_INET (domain)
TCP = SOCK_STREAM (type)
protocol = 0;
port -> serverconfig;
interface -> get listen from configParserbacklog -> hardcode 128? */

struct ClientCon
{
    int fd;
    uint32_t ip;
    uint16_t port;
    std::string remote_addr;
    uint16_t remote_port_net;
    time_t last_act = 0;

    HttpRequest req;
    const ServerConfig *servConf;

    std::string response_out;
    bool res_ready;
    bool keep_alive;
    size_t sent;
    bool cgi_active;
    bool cgi_force_close;
    bool close_after_send;

    ClientCon(int fd, uint32_t ip, uint16_t port):
        fd(fd),
        ip(ip),
        port(port),
        remote_port_net(0),
        last_act(std::time(NULL)),
        req(),
        servConf(nullptr),
        res_ready(false),
        keep_alive(false),
        sent(0),
        cgi_active(false),
        cgi_force_close(false),
        close_after_send(false)  {}
};

struct ListenSocket
{
    int fd;
    uint32_t ip;
    uint16_t port;

    ListenSocket(int fd, uint32_t ip, uint16_t port): fd(fd), ip(ip), port(port) {}
};

class Server
{
    private:
    const ConfigParser &config;
    std::unique_ptr<event::EventPoller> poller;
    std::unique_ptr<CgiManager> cgi_manager;
    int signal_pipe[2];
    bool signal_pipe_initialized;
    std::vector<ListenSocket> listen_sockets;
    std::unordered_set<int> listen_fd_set;
    std::unordered_map<int, ClientCon> clients;

    void new_connection(int listen_fd);
    void read_client(int client_fd);
    void client_write(int client_fd);
    const ServerConfig* findServer(uint32_t ip, uint16_t port, const std::string& host_header);
    void process_request(ClientCon& conn);
    void close_client(int client_fd);
    void check_timeouts();
    void fallback_error(ClientCon& conn, int status);

    int create_and_listen(const ListenPort& lp);
    void bind_and_listen(int fd, uint16_t port, uint32_t ip);
    void setup_signal_pipe();
    void handle_signal_pipe();
    void reap_children();
    void drain_client_backpressure(ClientCon& conn);
    void capture_peername(int client_fd, ClientCon& conn);

    void send_response(int client_fd, bool keep_alive);
    void send_error_response(int client_fd, int status_code, std::string error_info);

    const ListenSocket* get_listen_socket(int fd) const;
    bool dispatch_cgi_event(const event::PollEvent& ev);
    static void sigchld_handler(int);
    static Server* active_instance;

    // int dom, int typ, int prot, int port, u_long interface, int bl
    public:
    Server(ConfigParser &config);
    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = delete;
    ~Server();

    void init();
    void run();
};

#endif
