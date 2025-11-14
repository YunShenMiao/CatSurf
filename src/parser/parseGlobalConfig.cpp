#include "../../include/parser.hpp"

//set default values if empty

void Parser::parseGlobalDirective(const std::string& key, const std::string& value, Type t)
{
    if (key == "worker_processes" && t == NUMBER)
        global_config.worker_processes = std::stoi(value);
    else if (key == "error_log" && t == PATH)
        global_config.error_log = value;
    else if (key == "pid" && t == PATH)
        global_config.pid = value;
    else 
    {
        std::cerr << "Unknown global directive: " << key << "\n";
        exit(EXIT_FAILURE);
    }
}

void Parser::parseGlobalConfig(const std::vector<std::string>& tokens, size_t& i)
{
    while (i < tokens.size() && tokens[i] != "server") 
    {
        const std::string& key = tokens[i];
        if (i + 1 >= tokens.size())
        {
            std::cerr << "Missing value for global directive: " << key << "\n";
            exit(EXIT_FAILURE);
        }
        const std::string& value = tokens[i + 1];
        if (grammar[GLOBAL].find(key) != grammar[GLOBAL].end()) 
        {
            Type t = grammar[GLOBAL][key];
            if (!validateType(t, value))
            {
                std::cerr << "Invalid type for directive: " << key << "\n";
                exit(EXIT_FAILURE);
            }
            parseGlobalDirective(key, value, t);
        }
        else
        {
            std::cerr << "Invalid directive in global block: " << tokens[i] << "\n";
            exit(EXIT_FAILURE);
        }
        i += 2;
    }
    if (tokens[i] != "server")
    {
        std::cerr << "No server block found in config file\n";
            exit(EXIT_FAILURE);
    }
}