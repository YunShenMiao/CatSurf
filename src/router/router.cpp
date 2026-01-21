
#include "../../include/router.hpp"
#include "../../include/httpRequest.hpp"
#include "../../include/server.hpp"
#include <iostream>
#include <sys/stat.h>
#include <algorithm>

Router::Router(const ServerConfig& server, const parsedRequest &req): server(server), req(req) {}

Router::~Router() {}

bool Router::isCGI(const LocationConfig* loc, const std::string& file_path)
{
    if (!loc || loc->cgi_extension.empty())
        return false;
    size_t dot = file_path.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    
    std::string extension = file_path.substr(dot);
    for (size_t i = 0; i < loc->cgi_extension.size(); ++i)
    {
        if (extension == loc->cgi_extension[i])
        {
            result.cgi_ext = extension;
            return true;
        }
    }
    return false;
}

std::string normalizePath(const std::string& path)
{
    std::vector<std::string> parts;
    std::string current;

    for (size_t i = 0; i < path.size(); ++i)
    {
        if (path[i] == '/')
        {
            if (!current.empty())
            {
                if (current == "..")
                {
                    if (!parts.empty())
                        parts.pop_back();
                }
                else if (current != ".")
                    parts.push_back(current);
                current.clear();
            }
        }
        else
            current += path[i];
    }

    if (!current.empty())
    {
        if (current == "..")
        {
            if (!parts.empty())
                parts.pop_back();
        }
        else if (current != ".")
            parts.push_back(current);
    }

    std::string result = "/";
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            result += "/";
        result += parts[i];
    }

    return result;
}

// match location, check redirect, check allow methods, resolve path -> static | directory listing | cgi
Route Router::route()
{    
    const LocationConfig *loc = findLocation(req.uri);
    result.location = loc;
    if (loc && !loc->return_.empty())
	{
		result.type = RED;
		result.status = std::stoi(loc->return_[0]);
        result.redirect_url = loc->return_[1];
	}
    else if (loc && std::find(loc->allow_methods.begin(), loc->allow_methods.end(),
                req.method) == loc->allow_methods.end())
	{
		result.type = ERR;
		result.status = MethodNotAllowed;
	}
    else if (loc && (req.method != "GET") && !loc->upload_path.empty())
    {
        result.type = UPLOAD;
        return result;
    }

	else
	{
		std::string safe_uri = normalizePath(req.uri);
        result.file_path = mapURI(loc, safe_uri);
        if (result.file_path.empty())
        {
            result.type = ERR;
            result.status = Forbidden;
            return result;
        }

		struct stat st;
        if (stat(result.file_path.c_str(), &st) != 0)
        {
            result.type = ERR;
            result.status = NotFound;
        }
        else if (S_ISDIR(st.st_mode))
        {
            std::string index_path = mapIndexPath(result.file_path, loc);
        
            if (!index_path.empty())
            {
                result.file_path = index_path;
            
                if (isCGI(loc, index_path))
                {
                    if (loc->cgi_path.empty())
                    {
                        result.type = ERR;
                        result.status = InternalServerError;
                    }
                    else
                    {
                        result.type = CGI;
                        result.cgi_path = loc->cgi_path;
                    }
                }
                else
                    result.type = FILES;
            }
            else if (loc && loc->autoindex)
            {
                if (req.method != "GET")
                {
                    result.type = ERR;
                    result.status = MethodNotAllowed;
                }
                else
                result.type = DIRECTORY_LISTING;
            }
            else
            {
                result.type = ERR;
                result.status = Forbidden;
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            if (isCGI(loc, result.file_path))
            {
                if (loc->cgi_path.empty())
                {
                    result.type = ERR;
                    result.status = InternalServerError;
                }
                else
                {
                    result.type = CGI;
                    result.cgi_path = loc->cgi_path;
                }
            }
            else
                result.type = FILES;
        }
        else
        {
            result.type = ERR;
            result.status = Forbidden;
        }
    }
    return result;
}

std::string Router::mapIndexPath(const std::string& dir, const LocationConfig* loc) const
{
    std::vector<std::string> indexes;

    if (loc && !loc->index.empty())
        indexes = loc->index;
    else
        indexes = server.index;

    for (const std::string& idx : indexes)
    {
        std::string res = dir;
        if (res.back() != '/')
            res += '/';
        res += idx;

        struct stat st;
        if (stat(res.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            return res;
    }

    return "";
}


std::string Router::mapURI(const LocationConfig *loc, const std::string &uri)
{
    std::string root;
    if (loc && !loc->root.empty())
        root = loc->root;
    else
        root = server.root;

    if (!root.empty() && root.back() != '/')
        root += '/';
    
    std::string relative_path = uri;
    if (loc && uri.find(loc->path) == 0)
    {
        relative_path = uri.substr(loc->path.length());
        if (relative_path.empty() || relative_path[0] != '/')
            relative_path = "/" + relative_path;
    }

    relative_path = normalizePath(relative_path);
    if (!relative_path.empty() && relative_path[0] == '/')
        relative_path = relative_path.substr(1);

    std::string full_path = root + relative_path;
    if (full_path.size() < root.size() || full_path.compare(0, root.size(), root) != 0)
        return "";
    
    return full_path;
}

// match on bestmatch or exactmatch?
const LocationConfig* Router::findLocation(const std::string& uri) const
{
	if (server.locations.empty())
    	return nullptr;
  
  	const LocationConfig* best_match = nullptr;
  	size_t best_match_len = 0;
  
  	for (const auto& loc : server.locations)
  	{
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