#include "../../include/configParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
// parse config: look for file extension - order: global (or missing) - server must have listen, server_name; root & error & location optional { location{}}

ConfigParser::ConfigParser() {}

ConfigParser::~ConfigParser() {}

bool validateType(Type t, const std::vector<std::string>& value)
{
    if (value.empty())
        return false;
    switch(t)
    {
        case METH:
            for (size_t i = 0; i < value.size(); i++)
            {
                std::cout << isMethod(value[i]) << std::endl;
                if (!isMethod(value[i]))
                    return false;
            }
                return true;
        case DOMAIN:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (!isDomainname(value[i]))
                    return false;
            }
                return true;
        case FILENAME:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (!isFilename(value[i]))
                    return false;
            }
                return true;
        case CGI_EXT:
            for (size_t i = 0; i < value.size(); i++)
            {
                if (value[i][0] != '.')
                    return false;
                if (!isFilename(value[i].substr(1)))
                    return false;
            }
                return true;
        case MAP:
            if (value.size() < 2)
                return false;
            for (size_t i = 0; i < value.size() - 1; i++)
            {
                if (!isErrorCode(value[i]))
                    return false;
            }
            return isPath(value.back());
        case REDIRECT:
            return isRedirect(value);
        default:
            return false;
    }
}

bool validateType(Type t, const std::string& value)
{
    switch (t) 
    {
        case PORT:
            return isPort(value);
        case WORK_PRC:
            return isWorkerProcesses(value);
        case PATH:
            return isPath(value);
        case BOOLEAN:
            return isBoolean(value);
        case SIZE:
            return isSize(value);
        case TIME:
            return isTime(value);
        default:
            return false;
    }
}

bool validLine(std::string str)
{
    str.erase(std::remove_if(str.begin(), str.end(), 
              [](unsigned char c) { return std::isspace(c); }), 
              str.end());
    std::cout << str << std::endl;
    if (str == "server" || str == "server{" || str == "{" || str == "}")
        return true;
    if (str.find("location") != std::string::npos)
        return true;
    if (str.find(";") != str.size() - 1)
        return false;
    return true;
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
        if (!validLine(line))
            throw std::runtime_error("Invalid line syntax: " + line);

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