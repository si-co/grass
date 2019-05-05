//
// Created by quentin on 22/04/2019.
//

#ifndef GRASS_UTILS_HPP
#define GRASS_UTILS_HPP

#include "grass.hpp"
#include "sockutils.hpp"

typedef struct {
    bool runningPut;
    bool runningGet;
    bool interruptPut;
    bool interruptGet;
} Transfers;

// std::string helpers
std::deque<std::string> str_split (const std::string& input, const std::string& delimiter);
std::string str_join (std::vector<std::string> piecewise_str);
std::string str_join_q (std::deque<std::string> inputs, const std::string& delimiter);

// Execution primitives
std::string _check_cmd(const std::string& cmd);
void send_file_server(const std::string& path, int port, Transfers* transfers, Socket* mainSocket);
std::string send_file_from(Socket& sock, const std::string& fileName, Transfers* transfers, bool serv);
void recv_file_server(const std::string& path, long long size, int port, Transfers* transfers, Socket* mainSocket);
std::string recv_file_from(Socket& sock, char* bufferspace, const std::string& fileName, long long size, Transfers* transfers, bool serv);

void waitPut(Transfers* trans);
void waitGet(Transfers* trans);

// Path helpers
bool _sane_filename (const std::string& s);
bool _dir_exists (const std::string& path, const std::string& basedir="./");
bool _file_exists (const std::string& path, const std::string& basedir="./");
long long _file_size (const std::string& path, const std::string& basedir="");

// Data structure helpers
bool _is_authenticated(Session* usersmap, int clientFd);
void process_username(const char* username, size_t num);
void _stop_pending_auth(Session* usersmap, int clientFd);
std::string _error_authenticated(const std::string& cmd);
void _print_session (Session* s);

#endif //GRASS_UTILS_HPP
