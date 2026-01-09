#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>
#include <string>
#include "statusCodes.hpp"
#include "configParser.hpp"

// update host and stuff ya

class ServerConfig;
struct parsedRequest;

enum Resource
{
    RED,
    CGI,
    FILES,
    DIRECTORY_LISTING,
    ERR
};

struct Route
{
    Resource type;
    
    int status;
    std::string file_path;
    /* std::sring error_info; */
    std::string redirect_url;
    std::string cgi_path;
    std::string cgi_ext;
    const LocationConfig* location;
    
    Route() : type(ERR), status(0), location(nullptr) {}
};


class Router
{
	private:
    const ServerConfig &server;
	const parsedRequest &req;
    Route result;

	const LocationConfig* findLocation(const std::string& uri) const;
    bool isCGI(const LocationConfig* loc, const std::string& file_path);
	std::string mapURI(const LocationConfig *loc, const std::string &uri);
    std::string mapIndexPath(const std::string& dir, const LocationConfig* loc) const;
	
	public:
    Router(const ServerConfig& server, const parsedRequest& req);
    ~Router();
	Route route();
};

#endif