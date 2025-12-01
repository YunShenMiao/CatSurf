#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>
#include <optional>

class HttpRequest 
{
    private:
/*     std::string content; */
    std::string buffer;
    std::string method;
    std::string uri;
    std::string http_v;
    std::map<std::string, std::string> headers;
    std::string body;
    size_t content_length;
    bool is_complete;
    
    public:
    enum ParseState {REQUEST_LINE, HEADERS, BODY, COMPLETE, ERROR};
    ParseState state;
    //ocf
    HttpRequest();
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
    ~HttpRequest();

    ParseState parseRequest(const char* data, size_t len);
    void parseSL(std::string cont);
    void parseHeader(std::string cont);
    ParseState parseChunkedBody(std::string& buffer);
    
    // Getters
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string getHeaderVal(const std::string& key) const;

    //print
    void printRequest();
};

bool validateURI(std::string str);
bool validateHttpV(std::string str);
bool validateHeader(std::string key, std::string value);
void skipWhitespace(const std::string& str, size_t& i);

#endif