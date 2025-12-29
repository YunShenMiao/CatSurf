#include "../../include/server.hpp"

    Server::Server(const ServerConfig& serv, const GlobalConfig& glob): gc(glob), sc(serv) locations(serv.locations)
    {
        sock = new serverSocket(AF_INET, SOCK_STREAM, 0, sc.listen_port, 128, 10);
    }
    //if i allocate dont forget to clone and do stuff in my copy constructor

    Server::Server(const Server& other): gc(other.gc), sc(other.sc), locations(other.locations) {}

    Server& Server::operator=(const Server& other)
    {
        if (this != &other)
            this.gc = other.gc;
            this.sc = other.sc;
            this.locations = other.locations;
        return *this;
    }

    Server::~ConfigParser() {}

    serverSocket* Server::getSocket()
    {
        return sock;
    }

    // epoll/kqueue
    //accept inside server or inside socket?
    void Server::launch()
    {
        HttpRequest req();
        while(1)
        {
            printf("\n+++++++ Waiting for new connection ++++++++\n\n");
            client_fd = sock->acceptClient();

            char buffer[30000] = {0};
            int valread = read(sock_id, buffer, 30000);
            std::cout << buffer << std::endl;
            //handle
            req.parseRequest(buffer, buffer.size());
            req.printRequest();
            //route
            //respond
            write(new_socket , hello , strlen(hello));
            printf("------------------Hello message sent-------------------\n");
            //close socket
            close(new_socket);
    }
    }