#ifndef GRASS_H
#define GRASS_H

#define DEBUG true

// headers included with the skeleton
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>

// Additional headers from standard libraries
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <thread>
#include <arpa/inet.h>  // socket functionalities
#include <fcntl.h>      // socket functionalities
#include <ctime>
#include <iterator>
#include <sstream>
#include <algorithm>
#include <sys/epoll.h>

#include <errno.h>
#include <unistd.h>
#include <error.h>

#define UNUSED(expr) do { (void)(expr); } while (0)
#define SOCKET_TIMEOUT (1)
#define SOCKET_MSG_SIZE (4096)
#define CMD_OUTPUT_MAX_LEN (4096)
#define MAX_PATH_LEN (128)
#define FILE_CHUNK_SIZE (256)


typedef struct _User {
    std::string uname;
    std::string pass;
    int fd;              // file descriptor, initialized whenever a client logs in as the user. Serves as logged_in flag
    bool pending_login;  // 'awaiting password' flag, set to true after a login command
    bool logged_in;      // [...]
    // Need assingment operator
    struct _User& operator=(const _User& other) {
        if (this != &other) {
            //std::cout << "[_User] moving user " << other.uname << std::endl;
            this->uname = other.uname;
            this->pass = other.pass;
            this->fd = other.fd;
            this->pending_login = other.pending_login;
            this->logged_in = other.logged_in;
        }
        return *this;
    }
} User;

typedef struct _Command {
    std::string cmd;
    std::string arg1;
    std::string arg2;
    bool expects_output;
} Command;

typedef std::map<int, std::vector<User>> Session;

void hijack_flow();

#endif

