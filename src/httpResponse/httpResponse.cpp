//start line (status)
// header(Server, Date, Content-Type, Content-Length, Connection: keep-alive)
//body

#include "../../include/httpResponse.hpp"
#include "../../include/poller.h"
#include "../../include/utils.hpp"
#include <sstream>

HttpResponse::HttpResponse(std::string ka, std::string vers)
{
    http_v = vers;
    headers.insert({"Date", httpDate()});
    headers.insert({"Server", "CatSurf"});
    headers.insert({"Connection", ka});
    headers.insert({"Content-Length", "0"});
}
HttpResponse::~HttpResponse() {}

void HttpResponse::setHeader(std::string key, std::string value)
{
    auto it = headers.find(key);
    if (it != headers.end())
        headers[key] = value;
    else
        headers.insert({key, value});
}

void HttpResponse::setStatus(int stat)
{
    status_code = stat;
    status_info = mapStatus(stat);
}

void HttpResponse::setBody(std::string b)
{
    body = b;
}

std::string HttpResponse::buildResponse()
{
    std::ostringstream response;

    response << http_v << " "
             << status_code << " "
             << status_info << "\r\n";

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
        response << it->first << ": " << it->second << "\r\n";

    response << "\r\n";

    response << body;

    return response.str();
}

/* void HttpResponse::send_response(int client_fd)
{
    std::string response = buildResponse();

    const char *data = response.data();
    int remaining = static_cast<int>(response.size());

    while (remaining > 0)
    {
      int written = event::send_data(client_fd, data, remaining);
      if (written <= 0)
        break;
      data += written;
      remaining -= written;
    }
} */