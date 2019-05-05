#include "sockutils.hpp"

using namespace std;

/*  Documentation references :
 *
 *      addresses   http://man7.org/linux/man-pages/man7/address_families.7.html
 *
 *      socket()    http://man7.org/linux/man-pages/man2/socket.2.html
 *      bind()      http://man7.org/linux/man-pages/man2/bind.2.html
 *      listen()    http://man7.org/linux/man-pages/man2/listen.2.html
 *      accept()    http://man7.org/linux/man-pages/man2/accept.2.html
 *      connect()   http://man7.org/linux/man-pages/man2/connect.2.html
 *      close()     http://man7.org/linux/man-pages/man2/close.2.html
 *
 *      getSockOpt      http://man7.org/linux/man-pages/man2/setsockopt.2.html
 *                      http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_10_16
 *      getSockName()   http://man7.org/linux/man-pages/man2/getsockname.2.html
 */


// ===============================================================================================
//
//      CONSTRUCTORS / ASSIGNEMENTS / MOVE OPERATORS
//      ============================================
//

/*  Creates a IPV4 socket and bind it to a local ip:port
 *  Returns a file descriptor or an error code
 */
Socket::Socket (string localIp, int localPort, int timeout) {
    int res = 0;
    // 1. init sockaddr_in structure with desired ip:port
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;              // IPV4
    addr.sin_port = htons(localPort);       // htons = to binary format
    res = inet_pton(AF_INET, localIp.c_str(), &(addr.sin_addr));     // pton = copy address into structure with suitable binary format
    if (res <= 0) {
        throw runtime_error("Socket::Socket() : IP format Error");
    }
    // 2. Create socket object
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw runtime_error("Socket::Socket() : Unknown error");
    }
    // 3. Bind socket object to local ip:port using sockaddr_in
    //TEST
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        throw runtime_error("setsockopt(SO_REUSEADDR) failed");

    res = bind(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
    if ( res < 0) {
        close(sockfd);
        throw runtime_error("Socket::Socket() : Bind error");
    }
    // 4. Set timeout property
    if (timeout > 0) {
        struct timeval t;
        t.tv_sec = timeout;
        t.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    }
    this->localAddr = localIp;
    this->localPort = localPort;
    this->fd=sockfd;
    this->timeout = timeout;
}


/*  Create a socket Object from an existing C socket, via its file descriptor
 *  PRIVATE
 */
Socket::Socket (int newFd, int timeout) {
    // sockaddr_in parsing :
    // cf   https://stackoverflow.com/questions/1276294/getting-ipv4-address-from-a-sockaddr-structure
    //      https://linux.die.net/man/3/inet_ntoa

    //  1. Check for valid file descriptor
    int res;
    res = fcntl(newFd, F_GETFL);
    if (res == -1) {
        int err = errno;
        throw runtime_error ("Socket() : invalid file descriptor : "+to_string(newFd)+", error : "+strerror(err));
    }
    fd = newFd;

    // 2. Set timeout property
    if (timeout > 0) {
        struct timeval t;
        t.tv_sec = timeout;
        t.tv_usec = 0;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
    }

    //  2. Local address info
    struct sockaddr_in localSockaddr;
    socklen_t localAddrLen;
    getsockname(newFd, (struct sockaddr *) &localSockaddr, &localAddrLen);
    char* ip = inet_ntoa(localSockaddr.sin_addr);
    localAddr = string(ip);
    localPort = (int) localSockaddr.sin_port;

    //  3. Remote address info
    struct sockaddr_in remoteSockaddr;
    socklen_t remoteSockaddrLen = sizeof(remoteSockaddr);
    res = getpeername(newFd, (struct sockaddr*)&remoteSockaddr, &remoteSockaddrLen);
    if (res != 0 ) {
        int err = errno;
        throw runtime_error ("Socket() : unexpected error trying to obtain remote connection information : "+string(strerror(err)));
    }
    if (remoteSockaddr.sin_family != AF_INET ) {
        throw runtime_error ("Socket() : constructor only implemented for IPV4 (expected="+to_string(AF_INET)+", got="+to_string(remoteSockaddr.sin_family)+")");
    }
    ip = inet_ntoa(remoteSockaddr.sin_addr);
    remoteAddr = string(ip);
    remotePort = (int) remoteSockaddr.sin_port;
}

// Copy constructor
Socket::Socket(const Socket & from ) noexcept {
    remoteAddr = from.remoteAddr;
    remotePort = from.remotePort;
    localAddr = from.localAddr;
    localPort = from.localPort;
    fd = from.fd;
}

// move constructor
Socket::Socket(Socket && from) {
    if (this != &from) {
        // setup new object
        remoteAddr = from.remoteAddr;
        remotePort = from.remotePort;
        localAddr = from.localAddr;
        localPort = from.localPort;
        fd = from.fd;
        // empty old object
        from.localAddr = "x.x.x.x";
        from.remoteAddr = "x.x.x.x";
        from.localPort = -1;
        from.remotePort = -1;
        from.fd = -1;
    }
}

// move assignment
Socket& Socket::operator= (Socket && from) {
    if (this != &from) {
        // setup new object
        remoteAddr = from.remoteAddr;
        remotePort = from.remotePort;
        localAddr = from.localAddr;
        localPort = from.localPort;
        fd = from.fd;
        // empty old object
        from.localAddr="x.x.x.x";
        from.remoteAddr="x.x.x.x";
        from.localPort=-1;
        from.remotePort=-1;
        from.fd = -1;
    }
    return *this;
}

// copy assignment
// http://www.icu-project.org/docs/papers/cpp_report/the_anatomy_of_the_assignment_operator.html
//Socket& Socket::operator=(const Socket& from) {
//    if (this != &from) {
//        remoteAddr = from.remoteAddr;
//        remotePort = from.remotePort;
//        localAddr = from.localAddr;
//        localPort = from.localPort;
//        fd = from.fd;
//    }
//    return *this;
//}





// ===============================================================================================
//
//      PUBLIC FUNCTIONS
//      ================
//

int Socket::connect_ (string remoteIp, int newRemotePort) {
    int res = 0;
    //  1. sockaddr_in  for connect()
    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t) newRemotePort);
    res = inet_pton(AF_INET, remoteIp.c_str(), &addr.sin_addr);
    if (res < 0) {
        throw runtime_error("Socket::connect() : IP format Error");
    }
    //  2. connect()
    res = connect (fd, (struct sockaddr*)&addr, sizeof(addr));
    if (res < 0) {
        int err = errno;
        throw runtime_error("Socket::connect_() : System Error : ret="+to_string(res)+", err="+strerror(err));
    }
    // 3. update connection information
    remoteAddr = remoteIp;
    remotePort = newRemotePort;
    return 0;
}

Socket Socket::awaitClient () {
    int res = 0;
    int isListening = 0;
    socklen_t len = sizeof(isListening);
    // 1. If socket is not listenening, make it listen
    res = getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, &isListening, &len);
    if (res == -1) {
        throw runtime_error("Socket::awaitClient() : Unknown Error for fd="+to_string(fd));
    } else if (isListening == 0) {
        res = listen(fd, backlog);
        if (res != 0) {
            int err = errno;
            cout << "Error: Socket::awaitClient() returned an error!" << endl;
            close_();
            throw std::runtime_error("Socket::awaitClient() : fd="+to_string(fd)+" : System error during listen() : "+to_string(res)+" err="+strerror(err));
        }
    }
    // 2. accept client
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    int newFd = accept(fd, (struct sockaddr*) &clientAddr, &addrLen);
    if (newFd == -1) {
        int err = errno;
        throw std::runtime_error("Socket::awaitClient() : fd="+to_string(fd)+" : Error during accept() : ret="+to_string(newFd)+" err="+strerror(err));
    }
    // 3. Build and return new socket
    Socket ret {newFd, this->timeout};
    return ret;
}

ssize_t Socket::send_ (const void* sendBuffer, ssize_t len) {
    //  https://linux.die.net/man/2/send
    return send(fd, sendBuffer, len, 0);
}

ssize_t Socket::recv_ (void* recvBuffer, ssize_t len) {
    //  http://man7.org/linux/man-pages/man2/recv.2.html
    //  Possible interesting flags : MSG_DONTWAIT, MSG_PEEK
    return recv(fd, recvBuffer, len, NOFLAG);
}

ssize_t Socket::str_send(string sendstr) {
    return send(fd, sendstr.c_str(), sendstr.length(), 0);
}


string Socket::str_recv() {
    char buf[SOCKET_MSG_SIZE];
    int recvlen = SOCKET_MSG_SIZE;
    int rcvd = recv(fd, &buf, recvlen, NOFLAG);
    if (rcvd <= 0) {
        return string("");
    }
    return string(buf, rcvd);
}

void Socket::close_ () {
    // if invalid fd resource, do nothing
    // fd will be -1 if not bound, or after move operation
    if (fd>=0) {
        close(fd);
    } else {
    }
}




// ===============================================================================================
//
//      STATE INFORMATION
//      =================
//

bool Socket::isConnected () {
    return false;
}

int Socket::hasData() {
    return 0;
}


//
//  's' --> short string
//  'l' --> long string
//
string Socket::pprint (char opt) {
    string res = "Socket (fd="+to_string(fd);
    if (opt == 's') {
        //nothing
    } else if (opt == 'l') {
        res += ", local='" + localAddr + ":" + to_string(localPort) + "'";
        res +=", remote='" + remoteAddr + ":" + to_string(remotePort) + "'";
    }
    res += ")";
    return res;
}
