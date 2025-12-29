#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>

class serverSocket
{
    private:
    struct sockaddr_in adr;
    int server_fd;
    int lis;
    
    public:
    serverSocket(int dom, int typ, int prot, int port, u_long interface, int bl);
    serverSocket(const serverSocket &other);
    serverSocket& operator=(const serverSocket& other);
    ~serverSocket();

    struct sockaddr_in getSockadr();
    int getServFD();
    int getCon();

    /* void bindSocket(); */
    void listenSocket();
    int  acceptClient();
    void closeSocket();

};

#endif