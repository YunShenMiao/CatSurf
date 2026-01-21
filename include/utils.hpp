#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>

bool isNumber(const std::string& str);
bool isListen(const std::string& str);
bool isValidIP(const std::string& ip);
bool isPort(const std::string& str);
bool isMethod(const std::string& str);
bool isDomainname(const std::string& str);
bool isIPv6Host(const std::string& host);
bool isDirectory(const std::string& path);
std::string resolveConfigPath(const std::string& path);
bool isDefaultEP(int status);
std::string mapStatus(int code);
std::string httpDate();
std::string generateErrorPage(int status, std::string info);

#endif