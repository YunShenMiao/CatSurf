#include "../../include/httpRequest.hpp"
#include "../../include/configParser.hpp"
#include <iostream>
#include <cctype>
#include <algorithm>

// start line (METHOD URI HTTP/1.1 \r\n) (needs to have those + one space inbetween, no extra spaces (trailing, leading, inbetween))
// headers (Host, Content-Length, Transfer-Encoding) (key:(ows)value(ows)/r/n /r/n/r/n)
// body
// chunk example:
// 5\r\n
// hello\r\n
// 6\r\n
// world!\r\n
// 0\r\n
// \r\n
// 505 HTTP Version Not Supported for http version besides 1.0 & 1.1

/*
some error codes returned by parser
Condition	Parser error_code	Server responds
Invalid request line	400	400 Bad Request
Invalid headers	400	400 Bad Request
Unsupported HTTP version	505	505 HTTP Version Not Supported
Header fields too large	431	431 Request Header Fields Too Large
URI too long	414	414 URI Too Long
Invalid Content-Length	400	400 Bad Request
Body too large	413	413 Payload Too Large */
// i need to check my error handlong since i dpnt want to exit program if theres an error but handle it in response and go to response immediately

HttpRequest::HttpRequest (): content_length(0), error_code(0), is_complete(false), state(REQUEST_LINE) {}

HttpRequest::HttpRequest(const HttpRequest& other): buffer(other.buffer), method(other.method), uri(other.uri), http_v(other.http_v), headers(other.headers), body(other.body), content_length(other.content_length) , is_complete(other.is_complete), state(other.state) {}

HttpRequest& HttpRequest::operator=(const HttpRequest& other)
{
    if (this != &other)
    {
        buffer = other.buffer;
        method = other.method;
        uri = other.uri;
        http_v = other.http_v;
        headers = other.headers;
        body = other.body;
        content_length = other.content_length;
        is_complete = other.is_complete;
        state = other.state;
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

/***********************************************************/
/*                          GETTER                         */
/***********************************************************/

const std::string& HttpRequest::getMethod() const 
{
    return method;
}

const std::string& HttpRequest::getUri() const
{
    return uri;
}

const std::string HttpRequest::getHeaderVal(const std::string& key) const
{
    auto it = headers.find(key);
    if (it != headers.end())
        return it->second;
    return "";
}

/***********************************************************/
/*                          HELPER                         */
/***********************************************************/

std::string str_tolower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return s;
}

void HttpRequest::printRequest()
{
    std::cout << "=== Parsed Request ===\n";
    std::cout << "Method: " << method << "\n";
    std::cout << "URI: " << uri << "\n";
    std::cout << "Version: " << http_v << "\n";

    std::cout << "\nHeaders:\n";
    for (auto it = headers.begin(); it != headers.end(); ++it)
        std::cout << it->first << " => " << it->second << std::endl;

    std::cout << "======================\n";
}

void HttpRequest::printError()
{
    std::cout << "ErrorCode: " << error_code << std::endl;
    std::cout << "ErrorInfo: " << error_info << std::endl;
}

void HttpRequest::setError(ErrorCode type, std::string info)
{
    error_code = type;
    error_info = info;
    state = ERROR;
    throw std::runtime_error(info);
}

void skipWhitespace(const std::string& str, size_t& i)
{
    while (i < str.size() && (str[i] == ' ' || str[i] == '\t'))
        i++;
}

/***********************************************************/
/*                      VALIDATION                         */
/***********************************************************/

bool isMethod(const std::string& str)
{
    if (str.empty())
        return false;

    return str == "GET" || str == "POST" || str == "DELETE";
}

bool validateURI(std::string& str)
{
    if (str.empty() || str.size() > MAX_URI)
        return false;
    if (str[0] != '/')
        return false;

    std::string allowed = "-._~/?#&=:@!$\\()*+,;%";
    for (size_t i = 0; i < str.size(); i++)
    {
        if (!isalnum(static_cast<unsigned char>(str[i])) && allowed.find(str[i]) == std::string::npos)
            return false;
        if (str[i] == '%')
        {
            if (i + 2 >= str.size() || 
                !isxdigit(str[i+1]) || !isxdigit(str[i+2]))
                return false;
            i += 2;
        }
    } 
    return true;
}

bool validateHttpV(std::string str)
{
    if (str.empty())
        return false;
    else if (str != "HTTP/1.1" && str != "HTTP/1.0")
        return false;
    return true;
}

bool validateHeader(std::string key, std::string value)
{
    std::string allowed = "!#$%&'*+-.^_|~`";
    for (size_t i = 0; i < key.size(); i++)
    {
        if (!isalnum(static_cast<unsigned char>(key[i])) && allowed.find(key[i]) == std::string::npos)
            return false;
    }
    for (size_t i = 0; i < value.size(); i++)
    {
        if (!isprint(static_cast<unsigned char>(value[i])) && value[i] != '\t')
            return false;
    }
    return true;
}

/***********************************************************/
/*                        PARSING                          */
/***********************************************************/

void HttpRequest::parseSL(std::string cont)
{
    while (cont.size() >= 2 && cont[0] == '\r' && cont[1] == '\n')
        cont.erase(0, 2);

    size_t start = 0;
    size_t end = cont.find(' ', start);

    if (end == std::string::npos)
        setError(BadRequest, "Invalid Request line");
    method = cont.substr(start, end);
    if (!isMethod(method))
        setError(BadRequest, "Invalid method in Request line");

    start = end + 1;
    end = cont.find(' ', start);
    if (end == std::string::npos)
        setError(BadRequest, "Invalid Request line");
    uri = cont.substr(start, end - start);
    if (!validateURI(uri))
        setError(BadRequest, "Invalid URI in Request line"); 
    
    start = end + 1;
    http_v = cont.substr(start);
    if (!validateHttpV(http_v))
        setError(BadRequest, "Invalid HTTP version in Request line");
}

void HttpRequest::parseHeader(std::string cont)
{
    size_t i = 0;
    while (i < cont.size())
    {
        size_t key_end = cont.find(':', i);
        if (key_end == std::string::npos || key_end == i)
            setError(BadRequest, "Invalid header");
        std::string key = str_tolower(cont.substr(i, key_end - i));
        i = key_end + 1;
        skipWhitespace(cont, i);
        
        size_t value_end = cont.find("\r\n", i);

        std::string value = cont.substr(i, value_end - i);
        headers.insert({key, value});

        if (!validateHeader(key, value))
            setError(BadRequest, "Invalid header");
        if (value_end == std::string::npos || i == cont.size())
            break;
        if (value.size() > MAX_HEADER_LINE)
            setError(BadRequest, "Header line too long");

        i = value_end + 2;
    }
    if (http_v == "HTTP/1.1" && getHeaderVal("host").empty())
        setError(BadRequest, "Missing Host header");
    if (!getHeaderVal("content-length").empty())
    {
        try
        {
            long long cl = std::stoll(getHeaderVal("content-length"));
            if (cl < 0 || cl > MAX_CONT_LEN)
                setError(PayloadTooLarge, "Invalid Content-Length");
            content_length = static_cast<size_t>(cl);
        }
        catch (std::exception &e)
        {
            setError(BadRequest, "Invalid Content-Length");
        }
    }
}

//size chunk in hex, content chunk, ...
// no trailer header support
ParseState HttpRequest::parseChunkedBody(std::string& buffer)
{
    while (true)
    {
        size_t crlf = buffer.find("\r\n");
        if (crlf == std::string::npos)
            return BODY;

        std::string size_str = buffer.substr(0, crlf);
        size_t chunk_size;
        try
        {
            chunk_size = std::stoul(size_str, nullptr, 16);
        }
        catch (std::exception &e)
        {
            setError(BadRequest, "Invalid chunk size");
        }

        if (chunk_size == 0)
        {
            // do i need to consume final crlf as well ? || return body?
            buffer.erase(0, crlf + 2);
            return COMPLETE;
        }
        if (buffer.size() < crlf + 2 + chunk_size + 2)
            return BODY;
        if (buffer[crlf + 2 + chunk_size] != '\r' || buffer[crlf + 2 + chunk_size + 1] != '\n')
            setError(BadRequest, "malformed chunk");

        body.append(buffer, crlf + 2, chunk_size);
        buffer.erase(0, crlf + 2 + chunk_size + 2);
    }
}

ParseState HttpRequest::parseRequest(const char* data, size_t len)
{
    buffer.append(data, len);

    while (state != COMPLETE && state != ERROR)
    {
        if (state == REQUEST_LINE) 
        {
            if (buffer.size() > MAX_REQUEST_LINE)
            {
                setError(BadRequest, "invalid or too long request line");
                return state;
            }
            size_t pos = buffer.find("\r\n");
            if (pos == std::string::npos)
                return state;
            try
            {
                parseSL(buffer.substr(0, pos));
                buffer.erase(0, pos + 2);
                state = HEADERS;
            }
            catch (std::exception& e)
            {
                return state;
            }
        }
        else if (state == HEADERS) 
        {
            if (buffer.size() > MAX_HEADER_SIZE)
            {
                setError(BadRequest, "invalid header");
                return state;
            }
            size_t pos = buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
                return state;
            try
            {
                parseHeader(buffer.substr(0, pos));
                buffer.erase(0, pos + 4);
                if (content_length == 0 && getHeaderVal("transfer-encoding").find("chunked") == std::string::npos)
                    state = COMPLETE;
                else
                    state = BODY;
            }
            catch (std::exception &e)
            {
                return state;
            } 
        }
        else if (state == BODY) 
        {
            if (auto search = headers.find("transfer-encoding"); search != headers.end() && search->second == "chunked")
                state = parseChunkedBody(buffer);
            else if (buffer.size() >= content_length) 
            {
                body = buffer.substr(0, content_length);
                buffer.erase(0, content_length);
                state = COMPLETE;
            }
            return state;
        }
    }
    return COMPLETE;
}