#include "socket.hpp"

//binding only on server side (connect on client side)
/* socket()
bind()
listen()
accept()
recv() / read()
send() / write()
close() */

/* Good abstractions to build

create_listening_socket(port)

accept_client()

read_request(client_fd)

send_response(client_fd)

close_client(client_fd)

Later (mandatory):

non-blocking sockets

poll() / select() / epoll()

multiple clients */


serverSocket::serverSocket(int dom, int typ, int prot, int port, u_long interface, int bl)
{
    if ((server_fd = socket(domain, type, protocol)) < 0)
    {
        perror("failed to connect");
        exit(EXIT_FAILURE);
    }

    adr.sin_family = domain;
    adr.sin_port = htons(port);
    adr.sin_addr.s_addr = htonl(interface);
    backlog = bl;
    // (struct sockadr*)&adr ?
    if (bind(server_fd, &adr, sizeof(adr)) < 0) 
    {
        perror("failed to connect");
        exit(EXIT_FAILURE);
    }
}

serverSocket::serverSocket(const serverSocket &other): server_fd(other.server_fd) {}

serverSocket& serverSocket::operator=(const serverSocket& other)
{
    if (this != &other)
    {
        server_fd = other.server_fd;
    }
    return *this;
}

serverSocket::~serverSocket() {}

struct sockaddr_in serverSocket::getSockadr()
{
    return adr;
}
int serverSocket::getServFD()
{
    return server_fd;
}
int serverSocket::getCon()
{
    return connection;
}

void ServerSocket::listenSocket()
{
   //backlog max connections kinda????
    if ((lis = listen(server_fd, backlog)) < 0)
    {
        perror("failed to listen");
        exit(EXIT_FAILURE);
    }
}

int  acceptClient()
{
    if ((int  client_fd = accept(server_fd, (struct sockaddr *)&adr, (socklen_t*)(&(sizeof(adr))))) < 0)
    {
        perror("In accept");
        exit(EXIT_FAILURE);
    }
    return client_fd;
}

void closeSocket()
{

}
 



// AF_INET (IP) just int domain if we dont want to specify
// SOCK_RAW / SOCK_DGRAM /SOCK_STREAM...
    {
        int server_fd = socket(domain, type, protocol);
#
        int server_id = bind(server_fd, const struct sockaddr *address, socklen_t address_len);
    }