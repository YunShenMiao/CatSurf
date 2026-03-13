// Beispiel-Server
#include "poller.h"

#include <array>
#include <iostream>
#include <string>
#include <unordered_set>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace
{

  void bind_and_listen(int fd, uint16_t port)
  {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
      throw std::runtime_error("bind failed");
    }

    if (listen(fd, SOMAXCONN) != 0)
    {
      throw std::runtime_error("listen failed");
    }
  }

  void send_response(int client_fd)
  {
    const std::string body = "Miau Miauu Miauuuu!!!!!\n";
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: " +
        std::to_string(body.size()) + "\r\n\r\n" + body;

    const char *data = response.data();
    int remaining = static_cast<int>(response.size());

    while (remaining > 0)
    {
      int written = event::send_data(client_fd, data, remaining);
      if (written <= 0)
      {
        break;
      }
      data += written;
      remaining -= written;
    }
  }

}

int main()
{
  try
  {
    constexpr uint16_t kPort = 8081;

    // Create and setup listening socket
    int listen_fd = event::create_socket();
    event::set_socket_reuse(listen_fd);
    bind_and_listen(listen_fd, kPort);
    event::set_non_blocking(listen_fd);

    auto poller = event::make_poller();
    poller->add(listen_fd, true, false);

    std::unordered_set<int> clients;

    std::cout << "Listening on http://0.0.0.0:" << kPort << "\n";

    while (true)
    {
      for (const auto &event : poller->wait(1000))
      {
        if (event.fd == listen_fd && event.readable)
        {
          // Accept all pending connections
          while (true)
          {
            int client_fd = event::accept_connection(listen_fd);
            if (client_fd == -1)
            {
              break; // No more connections
            }
            event::set_non_blocking(client_fd);
            poller->add(client_fd, true, false);
            clients.insert(client_fd);
          }
        }
        else if (event.readable)
        {
          // Read from client
          std::array<char, 1024> buffer{};
          int bytes = event::receive_data(event.fd, buffer.data(), static_cast<int>(buffer.size()));

          if (bytes <= 0)
          {
            // Connection closed or error
            poller->remove(event.fd);
            clients.erase(event.fd);
            event::close_socket(event.fd);
            continue;
          }

          // Send response and close
          send_response(event.fd);
          poller->remove(event.fd);
          clients.erase(event.fd);
          event::close_socket(event.fd);
        }
      }
    }
  }
  catch (const std::exception &ex)
  {
    std::cerr << "Server error: " << ex.what() << "\n";
    return 1;
  }
}
