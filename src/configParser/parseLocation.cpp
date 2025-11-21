#include "../../include/configParser.hpp"
#include <set>

void ConfigParser::setLocationDefaults(ServerConfig& serv)
{
    for (size_t i = 0; i < serv.locations.size(); i++)
    {
        if (serv.locations[i].root.empty())
            serv.locations[i].root = serv.root;
        if (serv.locations[i].index.empty())
            serv.locations[i].index = serv.index;
        if (serv.locations[i].allow_methods.empty())
            serv.locations[i].allow_methods = {"GET"};
        if (serv.locations[i].client_max_body_size == 0)
            serv.locations[i].client_max_body_size = serv.client_max_body_size;
    }
}

void ConfigParser::setLocDirective(const std::string& key, const std::string& value, Type t, LocationConfig& loc)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key + " inside Location Block");
    if (key == "root")
        loc.root = value;
    else if (key == "autoindex")
    {
        if (value == "on")
            loc.autoindex = 1;
    }
    else if (key == "client_max_body_size")
        loc.client_max_body_size = parseSize(value);
    else if (key == "cgi_path")
        loc.cgi_path = value;
    else if (key == "upload_path")
        loc.upload_path = value;
}

void ConfigParser::setLocDirective(const std::string& key, const std::vector<std::string>& value, Type t, LocationConfig& loc)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key);
    if (key == "index")
        loc.index = value;
    else if (key == "allow_methods")
        loc.allow_methods = value;
    else if (key == "cgi_extension")
        loc.cgi_extension = value;
    else if (key == "return")
        loc.return_ = value;
}

void ConfigParser::parseLocation(const std::vector<std::string>& tokens, size_t& i, ServerConfig& serv)
{
    i++;
    if (i >= tokens.size() || !isLocationPath(tokens[i]))
        throw std::runtime_error("invalid path for Location Block");

    LocationConfig loc;
    loc.path = tokens[i];
    i++;

    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("ConfigSyntaxError: expected '{' after 'server'");
    i++;

    std::set<std::string> duplicateCheck;

    while (i < tokens.size() && tokens[i] != "}")
    {
        const std::string& key = tokens[i]; 
        if (duplicateCheck.count(key) > 0)
            throw std::runtime_error("Duplicate directive: " + key);
        duplicateCheck.insert(key); 

        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for directive: " + key);
        else if (grammar.at(LOCATION).find(key) != grammar.at(LOCATION).end())
        {
            Type t = grammar.at(LOCATION).at(key);
            i++;

            if (t == FILENAME || t == METH || t == CGI_EXT || t == REDIRECT)
            {
                std::vector<std::string> values;
                while (i < tokens.size() && tokens[i] != ";")
                {
                    values.push_back(tokens[i]);
                    i++;
                }
                if (i >= tokens.size() || tokens[i] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                setLocDirective(key, values, t, loc);
                i++;
            }
            else
            {
                const std::string& value = tokens[i];
                if (i + 1 >= tokens.size() || tokens[i + 1] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                if (!validateType(t, value))
                    throw std::runtime_error("Invalid type for directive: " + key + " inside Location Block");
                setLocDirective(key, value, t, loc);
                i += 2;
            }
        }
        else
            throw std::runtime_error("Unknown directive in Location Block: " + key);
    }
     if (i >= tokens.size() || tokens[i] != "}")
        throw std::runtime_error("Unclosed location block");
    i++;
    serv.locations.push_back(loc);
}
