#ifndef PARSER_H
#define PARSER_H


#include "grass.hpp"
#include "commands.hpp"

void parseInto (Command* cmd, const std::string& clientInput);

std::string dispatch (const std::string& basedir, char* pwd, Session* usersmap, Socket* clientFd, Command*, Transfers*);




# endif
