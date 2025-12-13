#include "../../include/server.hpp"

    Server::Server(const ServerConfig& serv, const GlobalConfig& glob): gc(glob), sc(serv) locations(serv.locations) {}

    Server::Server(const Server& other): gc(other.gc), sc(other.sc), locations(other.locations) {}

    Server& Server::operator=(const Server& other)
    {
        if (this != &other)
            this.gc = other.gc;
            this.sc = other.sc;
            this.locations = other.locations;
        return *this;
    }

    Server::~ConfigParser() {}
    