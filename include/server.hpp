#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector.hpp>
#include "configParser.hpp"
#include "serverSocket.hpp"

IPv4 = AF_INET (domain)
TCP = SOCK_STREAM (type)
protocol = 0;
port -> serverconfig;
interface -> get listen from configParserbacklog -> hardcode 128?
class Server
{
    private:
    serverSocket *sock;
    int client_fd;
    GlobalConfig gc;
    ServerConfig sc;
    std::vector<LocationConfig> locations;

    // int dom, int typ, int prot, int port, u_long interface, int bl
    public:
    Server(const ServerConfig& serv, const GlobalConfig& glob);
    Server(const Server& other);
    Server& operator=(const Server& other);
    ~Server();

    serverSocket* getSocket();
    void launch();
}