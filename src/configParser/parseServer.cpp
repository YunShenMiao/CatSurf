
#include "../../include/configParser.hpp"
#include <set>

void ConfigParser::setServerDefaults(ServerConfig &serv)
{
    if (serv.root.empty())
        serv.root = "/var/www/html";
    if (serv.server_name.empty())
        serv.server_name = {"_"};
    if (serv.index.empty())
        serv.index = {"index.html"};
    if (serv.error_page.empty())
        serv.error_page = {{404, "/404.html"}, {500, "/50x.html"}};
    if (serv.client_max_body_size == 0)
        serv.client_max_body_size = 1024 * 1024;
    if (serv.timeout == 0)
        serv.timeout = 60;
}

void ConfigParser::setServerDirective(const std::string& key, const std::string& value, Type t, ServerConfig& serv)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key + " inside Server Block");
    if (key == "listen")
    {
        if (isPort(value))
            serv.listen_port.push_back(ListenPort{static_cast<uint16_t>(std::stoi(value)), INADDR_ANY});
        else
        {
            size_t colon = value.find(':');
            uint16_t port = static_cast<uint16_t>(std::stoi(value.substr(colon + 1)));
            uint32_t ip = parseIPv4(value.substr(0, colon));
            serv.listen_port.push_back(ListenPort{port, ip});
        }
    }
    else if (key == "root")
        serv.root = value;
    else if (key == "client_max_body_size")
        serv.client_max_body_size = parseSize(value);
    else if (key == "timeout")
        serv.timeout = stoi(value);
}

void ConfigParser::setServerDirective(const std::string& key, const std::vector<std::string>& value, Type t, ServerConfig& serv)
{
    if (!validateType(t, value))
        throw std::runtime_error("Invalid value for directive: " + key);
    if (key == "index")
        serv.index = value;
    else if (key == "server_name")
        serv.server_name = value;
    else if (key == "error_page")
    {
        const std::string& path = value.back();
        for (size_t i = 0; i < value.size() - 1; i++)
        {
            int code = std::stoi(value[i]);
            serv.error_page[code] = path;
        }
    }
}

void ConfigParser::parseServer(const std::vector<std::string>& tokens, size_t& i)
{
    i++;
    if (i >= tokens.size() || tokens[i] != "{")
        throw std::runtime_error("ConfigSyntaxError: expected '{' after 'server'");
    i++;

    ServerConfig serv{};
    std::set<std::string> duplicateCheck;

    while (i < tokens.size() && tokens[i] != "}")
    {
        const std::string& key = tokens[i]; 

        if (duplicateCheck.count(key) > 0 && key != "location" && key != "listen" && key != "error_page")
            throw std::runtime_error("Duplicate directive: " + key);
        duplicateCheck.insert(key); 

        if (i + 1 >= tokens.size())
            throw std::runtime_error("Missing value for directive: " + key);
        if (key == "location")
            parseLocation(tokens, i, serv);
        else if (grammar.at(SERVER).find(key) != grammar.at(SERVER).end())
        {
            Type t = grammar.at(SERVER).at(key);

            if (t == DOMAIN|| t == FILENAME || t == MAP)
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
                setServerDirective(key, values, t, serv);
                i++;
            }
            else 
            {
                const std::string& value = tokens[i + 1];
                if (i + 2 >= tokens.size() || tokens[i + 2] != ";")
                    throw std::runtime_error("Syntax error: Missing ';'");
                if (!validateType(t, value))
                    throw std::runtime_error("Invalid type for directive: " + key);
                setServerDirective(key, value, t, serv);
                i += 3;
            }
        }
        else
            throw std::runtime_error("Unknown directive in server block: " + key);
    }
    
    if (i >= tokens.size() || tokens[i] != "}")
        throw std::runtime_error("Unclosed server block");
    i++;
    if (serv.listen_port.empty())
        throw std::runtime_error("Missing required 'listen' directive in server block");
    
    setServerDefaults(serv);
    setLocationDefaults(serv);
    servers.push_back(serv);
}
