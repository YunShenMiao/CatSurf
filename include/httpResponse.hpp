#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse 
{
    private:
    int status_code;
    std::string status_info;
    std::string http_v;
    std::map<std::string, std::string> headers;
    std::string body;

    public:
    HttpResponse(std::string ka, std::string vers);
    ~HttpResponse();
    
    void setHeader(std::string key, std::string value);
    void setStatus(int stat);
    void setBody(std::string b);
    std::string buildResponse();
    void send_response(int client_fd);
};

#endif