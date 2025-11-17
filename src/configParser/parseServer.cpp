
#include "../../include/configParser.hpp"

void ConfigParser::setServerDirective(const std::string& key, const std::string& value, Type t)
{
    if (key == "listen" && t == NUMBER && isNumber(value))
        servers[server_amnt].listen_port = stoi(value);
    else if (key == "root" && t == PATH && isPath(value))
        servers[server_amnt].root = value;
    else 
        throw std::runtime_error("Unknown global directive || invalid directive value; key: " + key + "value: " + value);
}

void ConfigParser::setServerDirective(const std::string& key, const std::vector<string&> value, Type t, ServerConfig& serv)
{
    if (key == "index" && t == LIST)
        serv.index = value;
    else if (key == "server_name" && t == LIST)
        serv.server_name = value;
    else if (key == "error_page" && t == MAP)
    {
        if (value.size < 2 || !isNumber(value[0]) || !isPath(value.back()))
            throw std::runtime_error("error_page requires code and path");
        std::map<int, std::string> error;
        for (size_t i = 0; i < value.size() - 1; i++)
        {
            error.first = value[i];
            error.second = value.back();
        }
        serv.error_pages = error;
    }
    else 
        throw std::runtime_error("Unknown global directive: " + key);
}

void ConfigParser::parseServer(const std::vector<std::string>& tokens, size_t& i)
{
    ServerConfig serv;
    i++;
    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("ConfigSyntaxError: expected '{' after 'server'");
    i++;
    
    while (i < tokens.size() && tokens[i] != "}")
    {
        const std::string& key = tokens[i];
        
        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for directive: " + key);
        
        const std::string& value = tokens[i + 1];
        
        if (grammar.at(SERVER).find(key) != grammar.at(SERVER).end())
        {
            Type t = grammar.at(SERVER).at(key);

            if (key == location)
                parseLocation();

            else if (t == LIST || t == MAP)
            {
                std::vector<std::string> values;
                i++;
                while (i < tokens.size() && tokens[i] != ";")
                {
                    values.push_back(tokens[i]);
                    i++;
                }
                if (i >= tokens.size() || tokens[i] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                    setServerDirective(server, key, values, t);
            }
            else 
            {
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                if (!validateType(t, value))
                    throw std::runtime_error("Invalid type for directive: " + key);
                setServerDirective(server, key, value, t);
                i += 3;
            }
        }
        else
        {
            throw std::runtime_error("Unknown directive in server block: " + key);
        }
    }
    
    if (i >= tokens.size())
        throw std::runtime_error("Unclosed server block");
    i++;
    
    if (server.listen_port == 0)
        throw std::runtime_error("Missing required 'listen' directive in server block");
    
    /* setServerDefaults(server); */
    
    servers.push_back(serv);
}
