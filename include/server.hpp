#ifndef SERVER_HPP
#define SERVER_HPP

#include <array>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <ctime>

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
    time_t last_act = 0;

    HttpRequest req;
    const ServerConfig *servConf;

    std::string response_out;
    bool res_ready;
    bool keep_alive;
    size_t sent;

    ClientCon(int fd, uint32_t ip, uint16_t port): fd(fd), ip(ip), port(port), last_act(std::time(NULL)), req(), servConf(nullptr), res_ready(false), keep_alive(false), sent(0)  {}
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

    void send_response(int client_fd, bool keep_alive);
    void send_error_response(int client_fd, int status_code, std::string error_info);

    const ListenSocket* get_listen_socket(int fd) const;

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