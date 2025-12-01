#include "../../include/httpRequest.hpp"
#include "../../include/configParser.hpp"
#include <iostream>
#include <optional>

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

/* struct ParseResult {
    bool ok;                     // true if parsing succeeded
    int error_code;              // HTTP status code (0 if no error)
    Request request;             // filled only if ok == true
};

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

HttpRequest::HttpRequest (): state(ParseState::REQUEST_LINE) {}

HttpRequest::HttpRequest(const HttpRequest& other): buffer(other.buffer), method(other.method), uri(other.uri), http_v(other.http_v), headers(other.headers), body(other.body), content_length(other.content_length) , is_complete(other.is_complete) {}

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
    }
    return *this;
}

HttpRequest::~HttpRequest() {}

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
    auto it = headers.find("Content-Length");
    if (it != headers.end())
        return (headers.find(key))->second;
    return "";
}


void HttpRequest::printRequest()
{
    std::cout << "=== Parsed Request ===\n";
    std::cout << "Method: " << method << "\n";
    std::cout << "URI: " << uri << "\n";
    std::cout << "Version: " << http_v << "\n";

    std::cout << "\nHeaders:\n";
    for (const auto& h : headers)
        std::cout << "  " << h.first << ": " << h.second << "\n";

    std::cout << "======================\n";
}

bool isMethod(const std::string& str)
{
    if (str.empty())
        return false;

    return str == "GET" || str == "POST" || str == "DELETE";
}

bool validateURI(std::string str)
{
    if (str.empty())
        return false;
    return true;
}

bool validateHttpV(std::string str)
{
    if (str.empty())
        return false;
    return true;
}

// check specifically for valid method, valid uri & http version
void HttpRequest::parseSL(std::string cont)
{
    size_t i = 0;
    size_t start = 0;
    while (i < cont.size() && cont[i] != ' ')
        i++;
    if (i >= cont.size() || cont[i] != ' ')
        throw std::runtime_error("Invalid request line" + cont);
    method = cont.substr(start, i - start);
    if (!isMethod(method))
        throw std::runtime_error("Invalid method: " + method);
    i++;
    start = i;

    while (i < cont.size() && cont[i] != ' ')
        i++;
    if (i >= cont.size() || cont[i] != ' ')
        throw std::runtime_error("Invalid request line" + cont);
    uri = cont.substr(start, i - start);
    if (!validateURI(uri))
        throw std::runtime_error("Invalid URI: " + uri); 
    i++;

    http_v = cont.substr(i);
    if (!validateHttpV(http_v))
        throw std::runtime_error("Invalid HTTP version: " + http_v);
}

void skipWhitespace(const std::string& str, size_t& i)
{
    while (i < str.size() && (str[i] == ' ' || str[i] == '\t'))
        i++;
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

void HttpRequest::parseHeader(std::string cont)
{
    size_t i = 0;
    while (i < cont.size())
    {
        size_t key_end = cont.find(':', i);
        if (key_end == std::string::npos)
            throw std::runtime_error("Invalid header: missing colon");
        std::string key = cont.substr(i, key_end - i);
        i = key_end + 1;
        skipWhitespace(cont, i);
        if (i >= cont.size())
            throw std::runtime_error("");
        
        size_t value_end = cont.find("\r\n", i);
        if (value_end == std::string::npos)
            throw std::runtime_error("Invalid header: missing CRLF");
        
        std::string value = cont.substr(i, value_end - i);
        headers.insert({key, value});
        i = value_end + 2;
        if (!validateHeader(key, value))
            throw std::runtime_error("Invalid header in http request: " + key + ", " + value);
    }
}

HttpRequest::ParseState HttpRequest::parseChunkedBody(std::string& buffer)
{
    while (true)
    {
        size_t crlf = buffer.find("\r\n");
        if (crlf == std::string::npos)
            return BODY;
        //(hex)
        std::string size_line = buffer.substr(0, crlf);
        size_t chunk_size = std::stoul(size_line, nullptr, 16);
        
        if (chunk_size == 0)
        {
            buffer.erase(0, crlf + 2);
            return COMPLETE;
        }
        
        if (buffer.size() < crlf + 2 + chunk_size + 2)
            return BODY;
        
        body.append(buffer, crlf + 2, chunk_size);
        buffer.erase(0, crlf + 2 + chunk_size + 2);
    }
}

HttpRequest::ParseState HttpRequest::parseRequest(const char* data, size_t len)
{
    buffer.append(data, len);
    std::cout << "state :" << state << std::endl;

    while (state != COMPLETE && state != ERROR)
    {
        if (state == REQUEST_LINE) 
        {
            size_t pos = buffer.find("\r\n");
            if (pos == std::string::npos)
                return state;
            try
            {
                std::cout << "miao2" << std::endl; 
                parseSL(buffer.substr(0, pos));
                buffer.erase(0, pos + 2);
                state = HEADERS;
            }
            catch (std::exception& e)
            {
                state = ERROR;
                return state;
            }
        }
        else if (state == HEADERS) 
        {
            size_t pos = buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
                return state;
            try
            {
                std::cout << "miao3" << std::endl; 
                parseHeader(buffer.substr(0, pos));
                std::cout << "miao3.1" << std::endl; 
                buffer.erase(0, pos + 4);
                if (!getHeaderVal("content-length").empty())
                    content_length = std::stoi(getHeaderVal("content-length"));
                else
                    content_length = 0;
                if (content_length == 0 && getHeaderVal("transfer-encoding").find("chunked") == std::string::npos)
                    state = COMPLETE;
                else
                    state = BODY;
                std::cout << "ishere" << std::endl;
            }
            catch (std::exception &e)
            {
                state = ERROR;
                return state;
            } 
        }
        else if (state == BODY) 
        {
            if (auto search = headers.find("Transfer-Encoding"); search != headers.end() && search->second == "chunked")
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