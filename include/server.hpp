#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector.hpp>
#include "configParser.hpp"

class Server
{
    private:
    GlobalConfig gc;
    ServerConfig sc;
    std::vector<LocationConfig> locations;


    public:
    Server();
    Server(const ServerConfig& serv, const GlobalConfig& glob);
    Server(const Server& other);
    Server& operator=(const Server& other);
    ~ConfigParser();
}