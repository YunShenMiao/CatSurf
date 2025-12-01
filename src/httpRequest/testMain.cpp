#include <iostream>
#include <string>
#include <map>
#include "../../include/httpRequest.hpp"

std::string testRequest =
    "GET /hello/world HTTP/1.1\r\n"
    "\r\n\r\n"
    "Host: example.com\r\n"
    "User-Agent: CustomClient/1.0\r\n"
    "Accept: text/html, application/json\r\n"
    "X-Test-Header:   test value   \r\n"
    "\r\n";

int main()
{
    {
        std::cout << "--- Testing valid request ---\n";

        HttpRequest req;
        HttpRequest::ParseState st = req.parseRequest(testRequest.c_str(), testRequest.size());

        if (st == HttpRequest::ParseState::COMPLETE)
            req.printRequest();
        else
            std::cerr << "Parser did not finish: state=" << (int)st << "\n";
    }

    std::cout << "\n-- Now testing malformed requests --\n";

    std::string bad1 =
        "GET / HTTP/1.1\r\n"
        "Bad Header Missing Colon\r\n"
        "\r\n";

    {
        HttpRequest r;
        try {
            auto st = r.parseRequest(bad1.c_str(), bad1.size());
            if (st == HttpRequest::ParseState::ERROR)
                throw std::runtime_error("Parser correctly reported ERROR");
            else
                std::cerr << "[FAIL] Did not detect malformed header\n";
        }
        catch (const std::exception& e) {
            std::cout << "[OK] Caught malformed header: " << e.what() << "\n";
        }
    }

    std::string bad2 =
        "GET / HTTP/1.1\r\n"
        "Host : example.com\r\n"   // INVALID: space before colon
        "\r\n";

    {
        HttpRequest r;
        try {
            auto st = r.parseRequest(bad2.c_str(), bad2.size());
            if (st == HttpRequest::ParseState::ERROR)
                throw std::runtime_error("Parser correctly reported ERROR");
            else
                std::cerr << "[FAIL] Did not detect space-before-colon\n";
        }
        catch (const std::exception& e) {
            std::cout << "[OK] Caught invalid header syntax: " << e.what() << "\n";
        }
    }

    std::string bad3 =
        "G ET / HTTP/1.1\r\n"      // INVALID method
        "Host: example.com\r\n"
        "\r\n";

    {
        HttpRequest r;
        try {
            auto st = r.parseRequest(bad3.c_str(), bad3.size());
            if (st == HttpRequest::ParseState::ERROR)
                throw std::runtime_error("Parser correctly reported ERROR");
            else
                std::cerr << "[FAIL] Did not detect invalid method\n";
        }
        catch (const std::exception& e) {
            std::cout << "[OK] Caught invalid method: " << e.what() << "\n";
        }
    }

    return 0;
}
