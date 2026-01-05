#include "../../include/httpRequest.hpp"
#include "../../include/configParser.hpp"
#include "../../include/utils.hpp"
#include <iostream>
#include <cctype>
#include <algorithm>


/*
1) CLOSE CONNECTION
depending on reason need to close connection after error (". If the unrecoverable error is in a request message, the server
respond with a 400 (Bad Request) status code and then close the connection. )")

2) PREFER TRANSFER OR RESULT IN ERROR?
TRANSFER ENCODING OVERWRITES CONTENT_LENGTH
--> A server reject a request that contains both Content-Length and Transfer-Encoding or
process such a request in accordance with the Transfer-Encoding alone. Regardless, the server
close the connection after responding to such a request to avoid the potential attacks.

3) ERROR CODES
501 for any transfer coding besides chunked !!!!!!! && http1.0 no transfer coding 
some error codes returned by parser
Condition	Parser error_code	Server responds
Invalid request line	400	400 Bad Request
Invalid headers	400	400 Bad Request
Unsupported HTTP version	505	505 HTTP Version Not Supported
Header fields too large	431	431 Request Header Fields Too Large
URI too long	414	414 URI Too Long
Invalid Content-Length	400	400 Bad Request
Body too large	413	413 Payload Too Large */


/* 
4) need to check again if this is about error oooor!!
A message that uses a valid Content-Length is
incomplete if the size of the message body received (in octets) is less than the value given by
Content-Length. A response that has neither chunked transfer coding nor Content-Length is
terminated by closure of the connection and, if the header section was received intact, is
considered complete unless an error was indicated by the underlying connection   p25*/

/*  
5) whould we handle? what would be excessive?
a server might reject traffic that it deems abusive or characteristic of a denial-of-service
attack, such as an excessive number of open connections from a single client.
 */

 // 6) right handling of connection closure 
 // if close from client, also close header in response & then close connection
 //last activity on recv -> HEADER_TIMEOUT     5–10 seconds BODY_TIMEOUT 10–30 seconds  KEEPALIVE_TIMEOUT  10–60 seconds


HttpRequest::HttpRequest (): content_length(0), error_code(0), chunked(false), is_complete(false), state(REQUEST_LINE) {}

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
        chunked = other.chunked;
        state = other.state;
        error_code = other.error_code;
        error_info = other.error_info;
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

parsedRequest HttpRequest::getRequest()
{
    parsedRequest req;
    req.buffer = buffer;
    req.method = method;
    req.uri = uri;
    req.http_v = http_v;
    req.headers = headers;
    req.body = body;
    req.content_length = content_length;
    req.error_code = error_code;
    req.error_info = error_info;
    return req;
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

// for event loop sth like this
/* std::vector<parsedRequest> requests;
if(state == COMPLETE)
{
    requests.push_back(obj.getRequest());
    obj.clear();
}
 */

void HttpRequest::clear()
{
    method.clear();
    uri.clear();
    http_v.clear();
    headers.clear();
    body.clear();
    content_length = 0;
    error_code = 0;
    error_info.clear();
    is_complete = false;
    chunked = false;
    state = REQUEST_LINE;
}

/***********************************************************/
/*                      VALIDATION                         */
/***********************************************************/

// maybe no \\?
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

void HttpRequest::check_host()
{
    if (http_v == "HTTP/1.1" && getHeaderVal("host").empty())
        setError(BadRequest, "Missing Host header");
    if (!getHeaderVal("host").empty())
    {
        std::string val = getHeaderVal("host");
        if (!isListen(val) && !isDomainname(val) && !isIPv6Host(val))
            setError(BadRequest, "Invalid host value");
    }
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
        setError(HTTPVersionNotSupported, "Invalid HTTP version in Request line");
}

void HttpRequest::check_cont_len()
{
    if (!getHeaderVal("content-length").empty())
    {
        try
        {
            if (!isNumber(getHeaderVal("content-length")))
                setError(BadRequest, "Invalid Content-Length");
            long long cl = std::stoll(getHeaderVal("content-length"));
            if (cl < 0)
                setError(BadRequest, "Invalid Content-Length");
            else if (cl > MAX_CONT_LEN)
                setError(PayloadTooLarge, "Payload too large");
            content_length = static_cast<size_t>(cl);
        }
        catch (std::exception &e)
        {
            setError(BadRequest, "Invalid Content-Length");
        }
    }
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
        if ((key == "host" && !getHeaderVal("host").empty()) || (key == "content-length" && !getHeaderVal("content-length").empty()))
            setError(BadRequest, "Multiple Host || Content-length header");
        
        size_t value_end = cont.find("\r\n", i);
        std::string value;
        if (value_end == std::string::npos)
            value = cont.substr(i);
        else
            value = cont.substr(i, value_end - i);
        if ((key.size() + value.size()) > MAX_HEADER_LINE)
            setError(BadRequest, "Header line too long");

        headers.insert({key, value});

        if (!validateHeader(key, value))
            setError(BadRequest, "Invalid header");
        if (value_end == std::string::npos || i == cont.size())
            break;

        i = value_end + 2;
    }
    if (!getHeaderVal("content-length").empty() && !getHeaderVal("transfer-encoding").empty())
        setError(BadRequest, "both header fields 'content-length' & 'transfer-encoding' present");
    check_transfer_enc();
    check_host();
    check_cont_len();
}

void HttpRequest::check_transfer_enc()
{
    if (!getHeaderVal("transfer-encoding").empty())
    {
        if ((str_tolower(getHeaderVal("transfer-encoding")).find("chunked") == std::string::npos))
            setError(BadRequest, "Unsupported Transfer-Encoding");
        else chunked = true;
    }
}

//size chunk in hex, content chunk, ...
// no trailer header support
// DO I NEED TO ADD MAX CHUNK SIZE && PARSE CHUNK SIZE? (rfc page 21), how is stoul handling 
// bad request if chunk size line includes ; (anything thats not valid hex)
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
        if (buffer.size() < crlf + 4)
            return BODY;
        buffer.erase(0, crlf + 4);
        return COMPLETE;
        }
        if (buffer.size() < crlf + 2 + chunk_size + 2)
            return BODY;
        if (buffer[crlf + 2 + chunk_size] != '\r' || buffer[crlf + 2 + chunk_size + 1] != '\n')
            setError(BadRequest, "malformed chunk");
        if (body.size() + chunk_size > MAX_CONT_LEN)
            setError(PayloadTooLarge, "body exceeds maximum size");
        body.append(buffer, crlf + 2, chunk_size);
        buffer.erase(0, crlf + 2 + chunk_size + 2);
    }
}

// need to add checks for payload too large, uri too long, request header fields too large
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
                // maybe just chunked check and then inside chunked need to see if error or notwith cont len, what had precedence?
                if (content_length == 0 && !chunked)
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
            if (chunked)
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