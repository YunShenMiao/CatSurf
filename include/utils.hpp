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

#endif