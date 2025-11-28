#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

class HttpRequest 
{
private:
/*     std::string content; */
    std::string method;
    std::string uri;
    std::string http_v;
    std::map<std::string, std::string> headers;
    std::string body;
    bool is_complete;
    
public:
    enum ParseState {REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR};
    //ocf
    HttpRequest();
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
    ~HttpRequest();

    ParseState parse(const std::string& data);
    
    // Getters
    const std::string& getMethod() const { return method; }
    const std::string& getUri() const { return uri; }
    const std::string& getHeader(const std::string& key) const;
};