#ifndef COMMANDS_H
#define COMMANDS_H

#include "grass.hpp"
#include "sockutils.hpp"
#include "utils.hpp"

std::string login   (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string pass    (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string ping    (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string ls      (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string cd      (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string mkdir   (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string rm      (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string get     (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string put     (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string grep    (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string date    (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string whoami  (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string w       (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string logout  (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);
std::string exit    (const std::string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans);

#endif
