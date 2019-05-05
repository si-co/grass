#include <grass.hpp>

#include "sockutils.hpp"
#include "parser.hpp"


/*
 * Server implementation for the grass project
 */


using namespace std;

static Session logged_users_map = map<int, vector<User>>(); // map<int, vector<User>>

string adress = "127.0.0.1";
int port = -1;
string basedir = "./";

// Server side REPL given a socket file descriptor
void* connection_handler(Socket clientSock) {
    // init base dir for this thread
    char currentWorkingDir[MAX_PATH_LEN+1]; // +1 char for string termination
    strncpy(currentWorkingDir, "", MAX_PATH_LEN+1);
    thread fileThread;
    Transfers transfers = {false, false, false, false};
    // REPL loop
    while (true) {
        // read input from client
        string recv = clientSock.str_recv();
        if (recv.length() == 0) {
            return 0;
        }
        // parse input
        Command cmd;
        try {
            parseInto(&cmd, recv);
        } catch (const std::invalid_argument& e) {
            char err[512];
            string strerr = string("Error: ")+e.what()+string("\n");
            strncpy(err, strerr.c_str(), 512);
            printf (err);
            fflush (stdout);
            clientSock.str_send("Error: "+string(e.what()));
            continue;
        }

        // dispatch command and send result
        string result = "";
        try {
            result = dispatch (basedir, currentWorkingDir, &logged_users_map, &clientSock, &cmd, &transfers);
        } catch (const std::runtime_error& e) {
            result  = "Error: runtime error" +string(e.what());
            cout << result<< endl;
        };

        if (result != "") {
            clientSock.str_send(result);
        }
    }
}

// Parse the grass.conf file and fill in the global variables
void parse_grass() {
    string line;
    ifstream conf ("grass.conf");

    // if impossible to open config file, then exit with error
    if (!conf.is_open()) {
        cout << "Error: unable to open config file" << endl;
        exit(1);
    }

    logged_users_map[-1] = vector<User>();
    while (getline(conf, line)) {
        size_t pos = 0;
        if ((pos = line.find(' ')) != string::npos) {
            // Read the keyword first
            string token = line.substr(0, pos);
            if (token.compare("base") == 0) {
                basedir = line.substr(pos+1, line.length());
            } else if (token.compare("port") == 0) {
                port = stoi(line.substr(pos+1, line.length()));
            } else if (token.compare("user") == 0) {
                string userString = line.substr(pos+1, line.length());
                if ((pos = userString.find(' ')) != string::npos) {
                    // read uname
                    string uname = userString.substr(0, pos);
                    // read password
                    string pass = userString.substr(pos+1, userString.length());
                    User user = {uname, pass, -1, false, false};
                    logged_users_map.at(-1).push_back(user);
                }
            }
        }
    }
    // make directory in all cases, then check it actually exists
    mkdir(basedir.c_str(), 0700);
    if (!_dir_exists(basedir)) {
        cout << "Error : base directory doesn't exist" << endl;
        exit(1);
    }

    conf.close();
}

int main() {
    // Parse the grass.conf file
    parse_grass();
    vector<thread> clients;
    Socket mainSocket {"127.0.0.1", port};

    // loop to accept commands by client
    while (true) {
        // wait for client via mainSocket
        Socket clientSock = mainSocket.awaitClient();

        // pass client file descriptor to connection_handler on new thread
        clients.push_back(
            thread {connection_handler, clientSock}
        );
        clients[clients.size()-1].detach();
    }
}
