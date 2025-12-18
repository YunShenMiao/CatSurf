#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>

// parse request - router - response

class Router
{
public:
    Router(const std::vector<ServerConfig>& servers);
    std::pair<const ServerConfig*, const LocationConfig*> route(const HttpRequest& request) const;

private:
    std::vector<ServerConfig> servers;
    const ServerConfig* matchServer(const std::string& host) const;
    const LocationConfig* matchLocation(const ServerConfig& server, const std::string& uri) const;
};

#endif