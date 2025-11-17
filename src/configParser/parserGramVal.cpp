#include "../../include/configParser.hpp"

// what inside locations?
// default for anything besides listen
// client_max_body_size?
/* root - defaults to some system path like /var/www/html
server_name - defaults to empty/catch-all
index - defaults to index.html
autoindex - defaults to off
client_max_body_size - defaults to 1m or similar
error_page - uses default error pages */

const std::map<Block, std::map<std::string, Type>> ConfigParser::grammar =
{
    {GLOBAL, 
        {
            {"worker_processes", NBR_AUTO},
            {"error_log", PATH},
            {"pid", PATH}
        }
    },
    {SERVER, 
        {
            {"listen", NUMBER},
            {"root", PATH},
            {"index", LIST},
            {"server_name", LIST},
            {"error_page", MAP},
            {"location", BLOCK}
        }
    },
    {LOCATION, 
        {
            {"root", PATH},
            {"autoindex", BOOLEAN},
            {"index", LIST},
            {"methods", LIST}
    }
    }
};

/* void ConfigParser::required() 
{
    if (!has("listen"))
        throw std::runtime_error("Missing required 'listen' directive");
} */

bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return str.find_first_not_of("0123456789") == std::string::npos;
}

bool isPath(const std::string& str)
{
    return !str.empty() && (str[0] == '/' || str[0] == '.');
}

bool isBoolean(const std::string& str)
{
    return str == "on" || str == "off";
}