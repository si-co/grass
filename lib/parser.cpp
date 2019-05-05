#include "parser.hpp"

/*  Parser + Dispatcher implementation
 */

using namespace std;


string valid_command_strings[] = {
    "login",
    "pass",
    "ping",
    "ls",
    "cd",
    "mkdir",
    "rm",
    "get",
    "put",
    "grep",
    "date",
    "whoami",
    "w",
    "logout",
    "exit"
};

map<string, string (*)(const std::string&, char*, Session*, Socket*, Command*, Transfers*)> dispatch_map = {

    {"login", &login},
    {"pass", &pass},
    {"ping", &ping},
    {"ls", &ls},
    {"cd", &cd},
    {"mkdir", &mkdir},
    {"rm", &rm},
    {"get",&get},
    {"put", &put},
    {"grep", &grep},
    {"date", &date},
    {"whoami", &whoami},
    {"w", &w},
    {"logout", &logout},
    {"exit", &exit}
};


// "private"
bool _isValidCommandString (const string& commandString) {
    for (string validString : valid_command_strings) {
        if (commandString == validString) {
            return true;
        }
    }
    return false;
}

int _nbParametersFor (const string& cmdstring) {
    if (! _isValidCommandString(cmdstring)) {
        throw invalid_argument ("Invalid command : "+cmdstring);
    }
    if (cmdstring == "put") {
        return 2;
    } else if ( cmdstring == "login" ||
                cmdstring == "pass" ||
                cmdstring == "ping" ||
                cmdstring == "cd" ||
                cmdstring == "mkdir" ||
                cmdstring == "rm" ||
                cmdstring == "get" ||
                cmdstring == "grep" ) {
        return 1;
    } else {
        return 0;
    }
}

bool _expects_output (const string& cmdstring) {
    if ( cmdstring == "date" ||
            cmdstring == "ping" ||
            cmdstring == "whoami" ||
            cmdstring == "w" ) {
        return true; // will necessarily have output
    }
    if( cmdstring == "login" ||
            cmdstring == "pass" ||
            cmdstring == "cd" ||
            cmdstring == "mkdir" ||
            cmdstring == "rm" ||
            cmdstring == "logout") {
        return true; // may have output on failure
    }
    return false;
}

//
//  Parse a string into a struct Command.
//      - No dynamic allocation
//      - throws exceptions for missing arguments and invalid command
//      - doesn't check content of arguments
//
void parseInto (Command* cmd, const string& clientInput) {

    //printf ("hijack_flow is %p \n", &hijack_flow);
    //printf ("parsing : ");
    //printf (clientInput.c_str());
    //printf ("\n");

    string remaining = clientInput;
    long unsigned int splitPos = remaining.length();

    // parse command
    splitPos = remaining.find(' ') == string::npos ? splitPos : remaining.find(' ');  // if find space, update split positions
    //cout << "debug : splitpos =" << to_string(splitPos) << endl;
    string cmdstring = remaining.substr(0, splitPos);
    if (!_isValidCommandString(cmdstring)) {
        //throw std::invalid_argument("Invalid cmd_str : "+cmdstring);
        throw std::invalid_argument("Invalid cmd_str : "+clientInput);
    }
    if (splitPos != remaining.length()) {
        remaining = remaining.substr(splitPos+1, remaining.length());
    } else {
        remaining = "";
    }

    // parse argument 1
    string arg1 = "";
    if (_nbParametersFor(cmdstring) >= 1) {
        splitPos = remaining.find(' ') == string::npos ? remaining.length() : remaining.find(' ');
        if (splitPos == 0) {
            throw std::invalid_argument("Empty argument 1 for command "+cmdstring);
        }
        arg1 = remaining.substr(0, splitPos);

        if (splitPos != remaining.length()) {
            remaining = remaining.substr(splitPos+1, remaining.length());
        } else {
            remaining = "";
        }
    }

    // parse argument 2
    string arg2 = "";
    if (_nbParametersFor(cmdstring) >= 2) {
        splitPos = remaining.find(' ') == string::npos ? remaining.length() : remaining.find(' ');
        if (splitPos == 0) {
            throw std::invalid_argument("Empty argument 2 for command "+cmdstring);
        }
        arg2 = remaining.substr(0, splitPos);
    }

    // update structure
    cmd->cmd = cmdstring;
    cmd->arg1 = arg1;
    cmd->arg2 = arg2;
    cmd->expects_output = _expects_output(cmdstring);
    return;
}


/*  Dispatcher
 *  Assumes that the command is a valid struct
 */
string dispatch (const string& basedir, char* pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    string cmdstring = cmd->cmd;
    if (! _isValidCommandString(cmdstring)) {
        throw invalid_argument("Invalid command string : " + cmdstring);
    }
    string result = "";
    result = dispatch_map[cmdstring](basedir, pwd, usersmap, clientSock, cmd, trans);
    return result;
}
