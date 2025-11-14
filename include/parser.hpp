#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <map>

/***********************************************************/
/*                       STRUCTS                           */
/***********************************************************/

struct GlobalConfig
{
    std::string worker_processes;
    std::string error_log_path;
    std::string pid_path;
};

struct LocationConfig
{
    std::string path;
    std::string root;
    bool allow_upload;
};

struct ServerConfig
{
    std::string server_name;
    int listen_port;
    std::string root;
    std::vector<LocationConfig> locations;
};

struct Config
{
    GlobalConfig global;
    std::vector<ServerConfig> servers;
};

/***********************************************************/
/*                       ENUM                              */
/***********************************************************/

enum Type
{
    NUMBER,
    PATH,
    BOOLEAN,
    STRING,
    LIST,
    MAP
}
enum Block
{
    GLOBAL,
    SERVER,
    LOCATION
}

/***********************************************************/
/*                       CLASS                             */
/***********************************************************/

class Parser
{
    private:
    static const std::map<std::map<Block, std::string>, Type> grammar;

    GlobalConfig global_config;
    std::vector<ServerConfig> servers;

    void parseGlobalConfig(std::string &path);
    void parseServerBlock(std::string &path);
    void required();

    public:
    void parse(const std::string& path);
    const GlobalConfig& getGlobalConfig() const;
    const std::vector<ServerConfig>& getServers() const;
};

/***********************************************************/
/*                  EXTERNAL FUNCTIONS                     */
/***********************************************************/


std::vector<std::string> tokenizeFile(const std::string& path);
bool validateType(Type t, std::string value);
bool isNumber(const std::string& str);
bool isPath(const std::string& str);
bool isBoolean(const std::string& str);

#endif