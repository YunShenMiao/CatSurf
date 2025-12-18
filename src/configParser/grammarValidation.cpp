#include "../../include/configParser.hpp"
#include "../../include/utils.hpp"
//return redirect catsurf.com -> catsurf.de (fast cgi pass) -> 
const std::map<Block, std::map<std::string, Type>> ConfigParser::grammar =
{
    {GLOBAL, 
        {
            {"error_log", PATH},
            {"pid", PATH}
        }
    },
    {SERVER, 
        {
            {"listen", PORT},
            {"root", PATH},
            {"index", FILENAME},
            {"server_name", DOMAIN},
            {"error_page", MAP},
            {"client_max_body_size", SIZE},
            {"timeout", TIME},
            {"location", BLOCK}
        }
    },
    {LOCATION, 
        {
            {"root", PATH},
            {"autoindex", BOOLEAN},
            {"index", FILENAME},
            {"allow_methods", METH},
            {"upload_path", PATH},
            {"cgi_extension", CGI_EXT},
            {"cgi_path", PATH},
            {"client_max_body_size", SIZE},
            {"return", REDIRECT}
        }
    }
};

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
            return isListen(value);
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

/* bool isMethod(const std::string& str)
{
    if (str.empty())
        return false;

    return str == "GET" || str == "POST" || str == "DELETE";
} */

/* bool isDomainname(const std::string& str)
{
    if (str.empty() || str.length() > 253 || str.find('/') != std::string::npos)
        return false;

    for (char c : str)
    {
        if (!std::isalnum(c) && c != '.' && c != '-' && c != ':' && c != '_')
            return false;
    }
    if (str[0] == '.' || str[0] == '-' || str.back() == '.' || str.back() == '-')
        return false;
    return true;
} */

bool isFilename(const std::string& str)
{
    if (str.empty() || str.find_first_of("/\0") != std::string::npos
        || str.length() > 255)
        return false;
    return true;
}

/* bool isNumber(const std::string& str)
{
    if (str.empty())
        return false;
    return (str.find_first_not_of("0123456789") == std::string::npos);
} */

bool isErrorCode(const std::string& str)
{
    if (!isNumber(str))
        return false;
    try
    {
        int code = std::stoi(str);
        return code >= 400 && code <= 599;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

/* bool isListen(const std::string& str)
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
} */

bool isPath(const std::string& str)
{
    if (str.empty())
        return false;
    if (str[0] == '/')
        return str.length() > 1;
    if (str.length() >= 2 && str[0] == '.' && str[1] == '/')
        return str.length() > 2;
    if (str.length() >= 3 && str[0] == '.' && str[1] == '.' && str[2] == '/')
        return str.length() > 3;
    return false;
}

bool isLocationPath(const std::string& str)
{
    if (str.empty() || str[0] != '/') 
    return false;

    if (str.find("..") != std::string::npos)
        return false;
    for (char c : str)
    {
        if (!std::isalnum(c) && c != '/' &&
            c != '-' && c != '.' && c != '_')
            return false;
    }
    return true;
}

bool isSize(const std::string& str)
{
    if (str.empty())
        return false;
    std::string numbers;
    char suffix = '\0';
    if (!std::isdigit(str.back()))
    {
        suffix = std::toupper(str.back());
        if (suffix != 'K' && suffix != 'M' && suffix != 'G')
            return false;
        numbers = str.substr(0, str.size() - 1);
    }
    else
        numbers = str;
    if (!isNumber(numbers))
        return false;
    try
    {
        long size = std::stol(numbers);
        if (size < 0)
            return false;
        if (suffix == 'G' && size > 10)
            return false;
        if (suffix == 'M' && size > 10240)
            return false;     
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

bool isBoolean(const std::string& str)
{
    return str == "on" || str == "off";
}

size_t parseSize(const std::string& str)
{
    std::string numbers;
    char suffix = '\0';
    
    if (!std::isdigit(str.back()))
    {
        suffix = std::toupper(str.back());
        numbers = str.substr(0, str.size() - 1);
    }
    else
        numbers = str;
    
    size_t size = std::stoul(numbers);
    switch (suffix)
    {
        case 'K': return size * 1024;
        case 'M': return size * 1024 * 1024;
        case 'G': return size * 1024 * 1024 * 1024;
        default: return size;
    }
}

bool isTime(const std::string& str)
{
    if (str.empty())
        return false;
    if (!isNumber(str))
        return false;
    try
    {
        int x = std::stoi(str);
        if (x < 1 || x > 86400)
            return false;
        return true;
    }
    catch (const std::exception& e)
    {
        return false;
    }
}

bool isRedirect(const std::vector<std::string>& values)
{
    if (values.size() != 2)
        return false;
    if (!isNumber(values[0]))
        return false;
    try
    {
        int code = std::stoi(values[0]);
/*         return code == 301 || code == 302 || code == 303 || 
               code == 307 || code == 308; */
            return code >= 100 && code <= 599;
    }
    catch (const std::exception&)
    {
        return false;
    }
    return isPath(values[1]) || isUrl(values[1]);
}

bool isUrl(const std::string& str)
{
    if (str.find("http://") == 0 || str.find("https://") == 0)
        return str.length() > 7;
    return false;
}