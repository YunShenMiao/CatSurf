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

HttpRequest::HttpRequest (const std::string content): request(content) {}

HttpRequest::~HttpRequest() {}

// check specifically for valid method, valid uri & http version
HttpRequest::parseSL(std::string cont)
{
    size_t i = 0;
    while (i < cont.size() && cont[i] != ' ')
    {
        method[i] == cont[i];
        i++;
    }
    i++;
    while (i < cont.size() && cont[i] != ' ')
    {
        uri[i] == cont[i];
        i++;
    }
    i++;
    size_t eol = cont.find("/r/n");
    http_v = cont.substr(i, eol);
    if (method.empty() || uri.empty() || http_v.empty())
        throw std::runtime_error("");
}

skipWhitespace(std::string str, size_t& i)
{
    while (str[i] == ' ' || str[i] == '\t')
        i++;
}

//check allowed ascii stuff in key and value str
validateHeader(std::string key, std::string value)
{

}

HttpRequest::parseHeader(std::string cont)
{
    while (i < cont.size() && i != cont.find("\r\n\r\n"))
    {
        size_t key_pos = content.find(':');
        key = content.substr(i, key_pos);
        i = key_pos + 1;
        skipWhitespace(content, i);

        if (i >= content.size())
            throw std::runtime_error("");

        size_t value_pos = content.find("\r\n");
        value = content.substr(i, value_pos);
        map.push_back({key, value});
        i = value_pos + 2;
        skipWhitespace(content, i);
        if (!validHeader(key, value))
            throw std::runtime_error("")
    }
}

ParseState HttpRequest::parseChunkedBody(std::string& buffer)
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

HttpRequest::ParseState parseRequest(const char* data, size_t len)
{
    buffer.append(data, len); 
    size_t i = 0;

    while (state != COMPLETE) 
    {
        if (state == REQUEST_LINE) 
        {
            size_t pos = buffer.find("\r\n");
            if (pos == std::string::npos)
                return state;
            parseSL(buffer.substr(0, pos));
            buffer.erase(0, pos + 2);
            state = HEADERS;
        }
        else if (state == HEADERS) 
        {
            size_t pos = buffer.find("\r\n\r\n");
            if (pos == std::string::npos)
                return state;
            parseHeaders(buffer.substr(0, pos));
            buffer.erase(0, pos + 4);
            state = BODY;
                
            content_length = getContentLength();
        }
        else if (state == BODY) 
        {
            if (auto search = headers.find("Transfer-Encoding"); search != headers.end() && search.second() == "chunked")
                state = parseChunkedBody(buffer);
            else if (buffer.size() >= content_length) 
                {
                    body = buffer.substr(0, content_length);
                    state = COMPLETE;
                }
            return state;
        }
    }
    return COMPLETE;
}