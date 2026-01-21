#include "../../include/utils.hpp"
#include "../../include/statuscodes.hpp"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sys/stat.h>
#include <limits.h>
#include <filesystem>
#include <ctime>

std::string httpDate()
{
    std::time_t now = std::time(NULL);

    std::tm gmt;
    gmt = *std::gmtime(&now);

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &gmt);

    return std::string(buffer);
}

std::string generateErrorPage(int status, std::string info)
{
    std::ostringstream html;

    html << "<!DOCTYPE html>\n"
         << "<html>\n"
         << "<head><title>"
         << status << " " << info
         << "</title></head>\n"
         << "<body>\n"
         << "<h1>" << status << " " << info << "</h1>\n"
         << "<p>Error occurred while processing your request.</p>\n"
         << "</body>\n"
         << "</html>\n";

    return html.str();
}


std::string mapStatus(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";

        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";

        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";

        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 505: return "HTTP Version Not Supported";

        default:  return "Unknown Status";
    }
}

bool isDefaultEP(int status)
{
    return status == BadRequest || status == Forbidden || status == NotFound
            || status == MethodNotAllowed || status ==PayloadTooLarge || status == InternalServerError
            || status == NotImplemented || status == HTTPVersionNotSupported;
}

bool isDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}

std::string resolveConfigPath(const std::string& path)
{
    if (path.empty())
        return path;

    std::filesystem::path p(path);
    if (p.is_relative())
        p = std::filesystem::current_path() / p;
    p = p.lexically_normal();

    return p.string();
}


bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return (str.find_first_not_of("0123456789") == std::string::npos);
}

bool isListen(const std::string& str)
{
    size_t colon = str.find(':');
    
    if (colon != std::string::npos)
    {
        std::string ip = str.substr(0, colon);
        std::string port = str.substr(colon + 1);
        
        return isValidIP(ip) && isPort(port);
    } 
    else
        return isPort(str);
}

bool isValidIP(const std::string& ip)
{
    if (ip.empty())
        return false;
    
    int dots = 0;
    for (char c : ip)
    {
        if (c == '.')
            dots++;
        else if (!std::isdigit(c))
            return false;
    }
    return dots == 3;
}

bool isPort(const std::string& str)
{
    if (!isNumber(str))
        return false;
    try 
    {
        int a = std::stoi(str);
        return a > 1 && a < 65535;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool isMethod(const std::string& str)
{
    if (str.empty())
        return false;

    return str == "GET" || str == "POST" || str == "DELETE";
}

// need to check again if it works for config & request now
bool isDomainname(const std::string& str)
{
    if (str.empty() || str.length() > 253 || str.find('/') != std::string::npos)
        return false;

    for (char c : str)
    {
        if (!std::isalnum(c) && c != '.' && c != '-'/*  && c != ':' && c != '_' */)
            return false;
    }
    if (str[0] == '.' || str[0] == '-' || str.back() == '.' || str.back() == '-')
        return false;
    return true;
}

bool isValidIPv6(const std::string& ip)
{
    if (ip.empty())
        return false;

    int ccount = 0;
    int dccount = 0;
    bool lastcol = false;

    for (size_t i = 0; i < ip.size(); ++i)
    {
        char c = ip[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
            lastcol = false;
        else if (c == ':')
        {
            if (lastcol)
            {
                dccount++;
                if (dccount > 1)
                    return false;
            }
            ccount++;
            lastcol = true;
        }
        else
            return false;
    }
    return ccount > 0;
}

// eg: [2001:db8::ff00:42:8329]     [::1]    [2001:0db8:0000:0000:0000:ff00:0042:8329]
bool isIPv6Host(const std::string& host)
{
    if (host.empty() || host.front() != '[')
        return false;
    size_t close = host.find(']');
    if (close == std::string::npos)
        return false;
    std::string ip = host.substr(1, close - 1);
    if (!isValidIPv6(ip))
        return false;
    if (close + 1 == host.length())
        return true;
    else if (host[close + 1] == ':')
    {
        std::string portStr = host.substr(close + 2);
        return isPort(portStr);
    }
    return false;
}
