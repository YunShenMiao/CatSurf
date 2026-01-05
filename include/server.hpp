#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include "configParser.hpp"
#include "poller.h"
#include "httpRequest.hpp"

#include <array>
#include <iostream>
#include <string>
#include <unordered_set>
#include <unordered_map>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

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
    time_t last_act;
    HttpRequest req;
    const ServerConfig *servConf;

    ClientCon(int fd, int ip, int port, time_t last_act): fd(fd), ip(ip), port(port), last_act(last_act), req() {}
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
    const ServerConfig* findServer(uint32_t ip, uint16_t port, const std::string& host_header);
    const LocationConfig* findLocation(const ServerConfig* server, const std::string& uri);
    void process_request(ClientCon& conn);
    void close_client(int client_fd);
    void check_timeouts();

    int create_and_listen(const ListenPort& lp);
    void bind_and_listen(int fd, uint16_t port, uint32_t ip);

    void send_response(int client_fd, bool keep_alive);
    void send_error_response(int client_fd, int status_code);

    const ListenSocket* get_listen_socket(int fd) const;

    // int dom, int typ, int prot, int port, u_long interface, int bl
    public:
    Server(ConfigParser &config);
    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = default;
    ~Server();

    void init();
    void run();
};

#endif