#ifndef SOCKUTILS_H
#define SOCKUTILS_H

#include "grass.hpp"

#define NOFLAG 0

enum SocketError {IpFormat, Bind, Connect, Other};

class Socket {
public :
    //  Public constructors
    Socket () {};
    Socket (std::string localIp, int localPort, int timeout=-1);   // From addr:port
    // destructor : !!! Only close socket explicitely !!!
    ~Socket () {};
    //  copy constructor, copy assignment
    Socket (const Socket &) noexcept;
    Socket& operator= (const Socket& from);
    // move constructor, move assignment
    Socket (Socket &&);
    Socket& operator= (Socket &&);
    //  Socket functionality :
    Socket awaitClient ();
    int connect_ (std::string remoteAddr, int remotePort);
    ssize_t send_ (const void* sendBuffer, ssize_t len);    // c-style, bytes
    ssize_t recv_ (void* recvBuffer, ssize_t len);
    ssize_t str_send(std::string sendstr);                  // c++ style, string
    std::string str_recv();
    void close_ ();
    // State information
    int getfd() {
        return fd;
    };
    bool isConnected ();
    int hasData ();
    // debug
    std::string pprint(char opt='s');

private :
    // fields
    int backlog {10};   // maximum number of pending (=connecting) connections
    int fd {-1};
    std::string localAddr {"x.x.x.x"};
    int localPort {-1};
    std::string remoteAddr {"x.x.x.x"};
    int remotePort {-1};
    int timeout {-1};
    bool connected {false};
    // Private constructor
    Socket (int fd, int timeout=-1);                               // From existing socket, via its file descriptor
};

#endif