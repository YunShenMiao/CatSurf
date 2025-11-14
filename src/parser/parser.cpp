#include "../../include/parser.hpp"
#include <iostream>
#include <fstream>
#include <vector>
// parse config: look for file extension - order: global (or missing) - server must have listen, server_name; root & error & location optional { location{}}

Parser::Parser() {}

Parser::~Parser() {}

bool validateType(Type t, std::string value)
{
    switch (t) 
    {
        case NUMBER:
            return isNumber(value);
        case PATH:
            return isPath(value);
        case BOOLEAN:
            return isBoolean(value);
        case STRING:
            return !token.empty();
        case LIST:
            // need to implement
        case MAP:
            // need to implement
        default:
            return false;
    }
}

void Parser::parseServer(std::vector<std::string> tokens, size_t& i)
{
    i++;
    if (tokens[i] != "{")
    {
        std::cerr << "invalid server block: missing '{' \n";
        exit(EXIT_FAILURE);
    }
    i++;
    while (i < tokens.size() && tokens[i] != "}")
    

}

/* for (size_t i = 0; i < tokens.size(); i++) {
    std::string token = tokens[i];
    
    if (token == "server") {
        // enter server block
        current_block = SERVER;
        expectToken("{");
    }
    else if (token == "}") {
        // exit block
    }
    else if (grammar[current_block].find(token) != grammar[current_block].end()) {
        // It's a valid directive! Parse it
        parseDirective(token);
    }
    else {
        throw std::runtime_error("Unknown token: " + token);
    }
} */

std::vector<std::string> tokenizeFile(const std::string& path)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::vector<std::string> tokens;
    std::string word;
    
    while (file >> word)
        tokens.push_back(word);
    
    return tokens;
}

void Parser::parse(const std::string& path)
{
    std::vector<std::string> tokens = tokenizeFile(path);
    size_t i = 0;

    while (i < tokens.size()) 
    {
        if (token != "server")
            parseGlobalConfig(tokens, i);
        else
            parseServer(tokens, i);
    }
}


const GlobalConfig& getGlobalConfig() const
{
    return global_config;
}
const std::vector<ServerConfig>& getServers() const
{
    return servers;
}