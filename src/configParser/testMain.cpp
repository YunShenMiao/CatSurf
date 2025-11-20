#include "../../include/configParser.hpp"
#define RED   "\033[31m"
#define BLUE  "\033[34m"
#define RESET "\033[0m"

/* void ConfigParser::test_print()
{
    std::cout << BLUE << "Global Config vals:" << std::endl;
    std::cout << RESET << "wp: " << global_config.worker_processes << " el: " << global_config.error_log << " pid: " << global_config.pid << std::endl;
    std::cout << RED << "------------------------------------------------" << std::endl;
    std::cout << BLUE <<"Server Config vals:" << std::endl;
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout << BLUE << "server" << i << ": " << RESET << std::endl;
        std::cout << "listen: " << servers[i].listen_port << " root: " << servers[i].root << std::endl;
        std::cout << "max_size: " << servers[i].client_max_body_size << std::endl;
        std::cout << "index: ";
        for (size_t j = 0; j < servers[i].index.size(); j++)
            std::cout << servers[i].index[j] << ", ";
        std::cout << std::endl;
        std::cout << "server_name: ";
        for (size_t j = 0; j < servers[i].server_name.size(); j++)
            std::cout << servers[i].server_name[j] << ", ";
        std::cout << std::endl;
        std::cout << "error_page: ";
        for (auto it = servers[i].error_page.begin(); it != servers[i].error_page.end(); it++)
            std::cout << it->first << " - " << it->second << ", ";
        std::cout << std::endl;
        std::cout << RED << "------------------------------------------------" << std::endl;
    for (size_t k = 0; k < servers[i].locations.size(); k++)
    {
    std::cout << BLUE << "Location vals:" << std::endl;
    std::cout << "location" << k << std::endl;
    std::cout << RESET << "path: " << servers[i].locations[k].path << ", root: " << servers[i].locations[k].root << std::endl;
    std::cout << "autoindex: " << servers[i].locations[k].autoindex << ", max_size: " << servers[i].locations[k].client_max_body_size << std::endl;
    std::cout << "cgi_path: " << servers[i].locations[k].cgi_path << std::endl;
    std::cout << "upload_path: " << servers[i].locations[k].upload_path << std::endl;
    std::cout << "index files:";
    for (size_t j = 0; j < servers[i].locations[k].index.size(); j++)
        std::cout << servers[i].locations[k].index[j] << ", ";
        std::cout << std::endl;
    std::cout << "cgi extension:";
    for (size_t j = 0; j < servers[i].locations[k].cgi_extension.size(); j++)
        std::cout << servers[i].locations[k].cgi_extension[j] << ", ";
        std::cout << std::endl;
    std::cout << "allowed methods: ";
    for (size_t j = 0; j < servers[i].locations[k].allow_methods.size(); j++)
        std::cout << servers[i].locations[k].allow_methods[j] << ", ";
    std::cout << std::endl;
    std::cout << RED << "------------------------------------------------" << std::endl;
    }
}

} */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "invalid amount of arguments: <programname + configfile>" << std::endl;
        return 1;
    }
    ConfigParser test;
    try
    {
        test.parse(argv[1]);
        test.test_print();
    }
    catch (const std::exception& e)
    {
        std::cerr << RED << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}