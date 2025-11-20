#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>

/***********************************************************/
/*                       STRUCTS                           */
/***********************************************************/

struct GlobalConfig
{
    unsigned int worker_processes = 0;
    std::string error_log;
    std::string pid;
};

struct LocationConfig
{
    std::string path;
    std::string root;
    std::string cgi_path;
    std::string upload_path;
    bool autoindex = 0;
    size_t client_max_body_size = 0;
    std::vector<std::string> cgi_extension;
    std::vector<std::string> return_;
    std::vector<std::string> index;
    std::vector<std::string> allow_methods;
};

struct ServerConfig
{
    std::vector<std::string> server_name;
    int listen_port = 0;
    std::string root;
    int timeout;
    size_t client_max_body_size = 0;
    std::vector<std::string> index;
    std::map<int, std::string> error_page;
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
    PORT,
    WORK_PRC,
    PATH,
    BOOLEAN,
    FILENAME,
    DOMAIN,
    MAP,
    BLOCK,
    SIZE,
    REDIRECT,
    TIME,
    CGI_EXT,
    METH
};

enum Block
{
    GLOBAL,
    SERVER,
    LOCATION
};

/***********************************************************/
/*                       CLASS                             */
/***********************************************************/

class ConfigParser
{
    private:
    static const std::map<Block, std::map<std::string, Type>> grammar;

    GlobalConfig global_config;
    std::vector<ServerConfig> servers;
    int server_amnt;

    void parseGlobalConfig(const std::vector<std::string>& tokens, size_t& i);
    void setDefaultGC();
    void setServerDefaults(ServerConfig &serv);
    void setLocationDefaults(ServerConfig& serv);
    void setServerDirective(const std::string& key, const std::string& value, Type t, ServerConfig& serv);
    void setServerDirective(const std::string& key, const std::vector<std::string>& value, Type t, ServerConfig& serv);
    void setLocDirective(const std::string& key, const std::string& value, Type t, LocationConfig& loc);
    void setLocDirective(const std::string& key, const std::vector<std::string>& value, Type t, LocationConfig& loc);
    void setGlobalDirective(const std::string& key, const std::string& value);
    void parseServer(const std::vector<std::string>& tokens, size_t& i);
    void parseLocation(const std::vector<std::string>& tokens, size_t& i, ServerConfig& serv);

    // Print helper
    void printGlobalConfig() const;
    void printServerConfig(const ServerConfig& server, size_t idx) const;
    void printLocation(const LocationConfig& loc, size_t idx) const;
    void printList(const std::vector<std::string>& list) const;
    void printMap(const std::map<int, std::string>& m) const;

    public:
    ConfigParser();
    ~ConfigParser();

    void parse(const std::string& path);
    const GlobalConfig& getGlobalConfig() const;
    const std::vector<ServerConfig>& getServers() const;
    void test_print();
};

/***********************************************************/
/*                  EXTERNAL FUNCTIONS                     */
/***********************************************************/


std::vector<std::string> tokenizeFile(const std::string& path);
bool validateType(Type t, const std::string& value);
bool validateType(Type t, const std::vector<std::string>& value);
bool isNumber(const std::string& str);
bool isPath(const std::string& str);
bool isBoolean(const std::string& str);
bool isFilename(const std::string& str);
bool isDomainname(const std::string& str);
bool isMethod(const std::string& str);
bool isPort(const std::string& str);
bool isErrorCode(const std::string& str);
bool isWorkerProcesses(const std::string& str);
bool isLocationPath(const std::string& str);
bool isSize(const std::string& str);
bool isUrl(const std::string& str);
bool isTime(const std::string& str);
bool isRedirect(const std::vector<std::string>& values);

bool validLine(std::string str);
size_t parseSize(const std::string& str);

#endif