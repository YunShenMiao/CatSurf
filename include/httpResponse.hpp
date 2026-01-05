class HttpResponse {
    int status_code;
    std::string status_message;
    std::map<std::string, std::string> headers;
    std::string body;
    
    // Convert to raw HTTP string for sending
    std::string toString() const;
    
    // Helper methods
    static HttpResponse ok(const std::string& body);
    static HttpResponse error(int code, const std::string& message);
    static HttpResponse redirect(const std::string& location);
};

class HttpResponse
{
    private:
        int status_code;
        std::map<std::string, std::string> headers;
        std::string body;
        std::string buffer;  // For partial sends
        size_t bytes_sent;
        
    public:
        void setStatus(int code);
        void setHeader(const std::string& key, const std::string& val);
        void setBody(const std::string& content);
        std::string serialize();  // Generate full response
        bool sendNonBlocking(int fd);  // Handle partial writes
};