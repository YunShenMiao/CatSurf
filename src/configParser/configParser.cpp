#include "../../include/configParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
// parse config: look for file extension - order: global (or missing) - server must have listen, server_name; root & error & location optional { location{}}

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

//i dont call this.. 
bool validateType(Type t, std::string value)
{
    switch (t) 
    {
        case NUMBER:
            return isNumber(value);
        case NBR_AUTO:
            return isNumber(value) || (value == "auto");
        case PATH:
            return isPath(value);
        case BOOLEAN:
            return isBoolean(value);
        case STRING:
            return !value.empty();
        case LIST:
            // need to implement
        case MAP:
            // need to implement
        default:
            return false;
    }
}

std::vector<std::string> tokenizeFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);
    
    std::vector<std::string> tokens;
    std::string line;

    while (std::getline(file,line))
    {
        size_t comment = line.find('#');
        if (comment != std::string::npos)
            line = line.substr(0, comment);

        std::istringstream iss(line);
        std::string word;

        while (iss >> word)
        {
            if (!word.empty() && word.back() == ';')
            {
                tokens.push_back(word.substr(0, word.size() - 1));
                tokens.push_back(";");
            }
            else
                tokens.push_back(word);
        }
    }
    return tokens;
}

void ConfigParser::parse(const std::string& path)
{
    std::vector<std::string> tokens = tokenizeFile(path);

    size_t i = 0;

    parseGlobalConfig(tokens, i);
    std::cout << "Global Config vals: wp: " << global_config.worker_processes << " el: " << global_config.error_log << " pid: " << global_config.pid << std::endl;

    while (i < tokens.size()) 
    {
        if (tokens[i] == "server")
            parseServer(tokens, i);
        else
            throw std::runtime_error("Unexpected token outside server block: " + tokens[i]);
    }
    if (servers.empty())
        throw std::runtime_error("No server blocks found in config file");
}

const GlobalConfig& ConfigParser::getGlobalConfig() const
{
    return global_config;
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
    return servers;
}