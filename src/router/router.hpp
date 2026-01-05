#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>
#include "../../httpRequest.hpp"

// parse request - router - response
//diconnect ya

class Router
{
	public:
    Router(const std::vector<ServerConfig>& servers, const HttpRequest& req);
    ~Router();
	
    std::pair<const ServerConfig*, const LocationConfig*> route(const HttpRequest& request) const;

	private:
    std::vector<ServerConfig> servers;
	std::string uri;
	std::map<std::string, std::string> headers;


    const ServerConfig* findServer(const std::string& host) const;
    const LocationConfig* findLocation(const ServerConfig& server, const std::string& uri) const;
};

#endif

Router::Router(const std::vector<ServerConfig>& servers): servers(servers) {}

Router::~Router() {}

std::pair<const ServerConfig*, const LocationConfig*> route(const HttpRequest& request) const
{
	const ServerConfig serv= findServer();
	const LocationConfig loc = findLocation(serv);
	return ({serv, loc})
}

// prio: match on IP & port & server name, else match on port& IP, fallback match on port
const ServerConfig* findServer(uint32_t ip, uint16_t port, const std::string& host_header)
{
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

// 
const LocationConfig* findLocation(const ServerConfig* server, const std::string& uri)
{
	if (!server || server->locations.empty())
    	return nullptr;
  
  	const LocationConfig* best_match = nullptr;
  	size_t best_match_len = 0;
  
  	for (const auto& loc : server->locations)
  	{
    // Check if URI starts with location path
    	if (uri.find(loc.path) == 0)
    	{
      		size_t match_len = loc.path.length();
      		if (match_len > best_match_len)
      		{
        		best_match = &loc;
        		best_match_len = match_len;
      		}
    	}
  	}
  	return best_match;
}