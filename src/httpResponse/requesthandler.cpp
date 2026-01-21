
#include "../../include/requesthandler.hpp"
#include "../../include/utils.hpp"
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdio>

// is files and error completed????
// handle files fully (is it fine to check file/directory in routing, should i change logic?
// check my error_response handling
// goal: finish static file serving & error pages + error response (go through and also adjust upload)
// + make plan what to continue (redirect??)
// automize header generation ???
//  also request parsing error earlier lead to server stopping, need to check on it

RequestHandler::RequestHandler(const Route &r, const parsedRequest &pr, const ServerConfig &sc, bool keep_alive): r(r), pr(pr), sc(sc) 
{
    if (keep_alive)
        ka = "keep-alive";
    else 
        ka = "close";
}

RequestHandler::~RequestHandler() {}

HttpResponse RequestHandler::handle()
{
    switch (r.type)
    {
        case RED:
            return handleRedirect();
        case CGI:
            return handleCGI();
        case FILES:
            return handleFiles();
        case UPLOAD:
            return handleFiles();
        case DIRECTORY_LISTING:
            return handleDirectoryListing();
        default:
            return handleError(r.status);
    }
}

HttpResponse RequestHandler::handleCGI() 
{
    HttpResponse res(ka, pr.http_v);
    return res;
}
HttpResponse RequestHandler::handleDirectoryListing()
{
    HttpResponse res(ka, pr.http_v);
    return res;
}

std::string getMime(std::string path)
{
    static std::map<std::string, std::string> mimeTypes;

    if (mimeTypes.empty())
    {
        mimeTypes["html"] = "text/html";
        mimeTypes["htm"]  = "text/html";
        mimeTypes["css"]  = "text/css";
        mimeTypes["js"]   = "application/javascript";
        mimeTypes["txt"]  = "text/plain";
        mimeTypes["png"]  = "image/png";
        mimeTypes["jpg"]  = "image/jpeg";
        mimeTypes["jpeg"] = "image/jpeg";
        mimeTypes["gif"]  = "image/gif";
        mimeTypes["ico"]  = "image/x-icon";
        mimeTypes["svg"]  = "image/svg+xml";
        mimeTypes["json"] = "application/json";
        mimeTypes["pdf"]  = "application/pdf";
    }

    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";
    std::string key = path.substr(dot + 1);
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    std::map<std::string, std::string>::iterator it = mimeTypes.find(key);
    if (it != mimeTypes.end())
        return it->second;

    return "application/octet-stream";
}

std::string extractFile(std::string filePath)
{
    size_t sep = filePath.find_last_of("/");
    if (sep != std::string::npos)
        return filePath.substr(sep + 1);
    return "";
}

//adjust in routing & in requesthandler
HttpResponse RequestHandler::deleteFile()
{
    if (!r.location)
        return handleError(NotFound);
    if (r.location->upload_path.empty())
        return handleError(MethodNotAllowed);
    else
    {
        std::string fileName = extractFile(pr.uri);
        if (fileName.empty())
            return handleError(NotFound);
        if (fileName == "." || fileName == ".." || fileName.find('\\') != std::string::npos)
            return handleError(BadRequest);
        std::string uploadPath = r.location->upload_path;
        if (uploadPath.back() != '/')
        uploadPath += '/';
        if (remove((uploadPath + fileName).c_str()) != 0)
        {
            if (errno == ENOENT)
                return handleError(NotFound);
            return handleError(Forbidden);
        }
        HttpResponse res(ka, pr.http_v);
        res.setStatus(NoContent);
        return res;
    }
}

HttpResponse RequestHandler::serveFile()
{
    std::string body;
    if (!readFile(r.file_path, body))
        return handleError(Forbidden);
    std::string contentType = getMime(r.file_path);
    if (contentType.find("text/") == 0 || contentType == "application/javascript")
        contentType += "; charset=utf-8";

    HttpResponse res(ka, pr.http_v);
    res.setHeader("Content-Type", contentType);
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setStatus(Ok);
    res.setBody(body);
    return res;
}

std::string generateFilename()
{
    static size_t count = 0;
    std::ostringstream oss;
    oss << std::time(nullptr) << "_" << count++;
    return oss.str();
}

HttpResponse RequestHandler::uploadFile()
{
    if (!r.location || r.location->upload_path.empty())
        return handleError(MethodNotAllowed);
    if (pr.body.empty())
        return handleError(BadRequest);
    if (r.location->client_max_body_size > 0 && pr.body.size() > r.location->client_max_body_size)
        return handleError(PayloadTooLarge);

    std::string fullPath = r.location->upload_path;
    if (fullPath.back() != '/')
        fullPath += '/';

    std::string fileName = generateFilename();
    std::ofstream filey(fullPath + fileName, std::ios::binary);
    if (!filey.is_open())
        return handleError(NotFound);
    filey << pr.body;
    filey.close();

    HttpResponse res(ka, pr.http_v);
    res.setStatus(Created);
    res.setHeader("Location", "/needUplaodPath/" + fileName);
    return res;
}

HttpResponse RequestHandler::handleFiles()
{
    if (r.type == UPLOAD && pr.method == "POST")
        return uploadFile();
    if (r.type == UPLOAD && pr.method == "DELETE")
        return deleteFile();
    if (!std::filesystem::exists(r.file_path))
        return handleError(NotFound);
    else
        return serveFile();
}

HttpResponse RequestHandler::handleError(int status)
{
    HttpResponse res(ka, pr.http_v);
    std::string body;
    std::map<int, std::string>::const_iterator it = sc.error_page.find(status);

    if (it != sc.error_page.end())
    {
        if (!readFile(sc.root + "/" + it->second, body))
            std::cout << "\n\n\nmiao\n\n\n\n";
    }
    else
        body = generateErrorPage(status, mapStatus(status));

    res.setHeader("Content-Type", "text/html");
    res.setHeader("Content-Length", std::to_string(body.size()));
    res.setStatus(status);
    res.setBody(body);
    return res;
}

bool readFile(const std::string& filepath, std::string& body)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open())
        return false;

    std::ostringstream buffer;
    buffer << file.rdbuf();
    body = buffer.str();
    return true;
}

HttpResponse RequestHandler::handleRedirect()
{
    HttpResponse res(ka, pr.http_v);
    
    res.setStatus(r.status);
    res.setHeader("Location", r.redirect_url);

    return res;
}