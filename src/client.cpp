#include <grass.hpp>
#include "sockutils.hpp"
#include "utils.hpp"
#include "parser.hpp"


/*
 * Client implementation for the grass project
 */


using namespace std;

string servAddress;
Transfers transfers;
string pendingPutFile = "";
string pendingGetFile = "";


/* Send a file to the server as its own thread (put)
 */
void send_file_client(string fileName, int d_port, Socket* mainSocket) {
    transfers.runningPut = true;
    Socket fileSock ("127.0.0.1", 0, SOCKET_TIMEOUT);
    fileSock.connect_(servAddress, d_port);
    string error = send_file_from(fileSock, fileName, &transfers, false);
    if (error != "") {
        cout << error << endl;
        mainSocket->str_send(error);
    }
    fileSock.close_();
    transfers.runningPut = false;
}

/* Recv a file from the server as its own thread (get)
 */
void recv_file_client(string fileName, int d_port, long long size, Socket* mainSocket) {
    transfers.runningGet = true;
    Socket fileSock ("127.0.0.1", 0, SOCKET_TIMEOUT);
    fileSock.connect_(servAddress, d_port);
    char buffer[FILE_CHUNK_SIZE];
    string error = recv_file_from(fileSock, buffer, fileName, size, &transfers, false);
    if (error != "") {
        cout << error << endl;
        mainSocket->str_send(error);
    }
    fileSock.close_();
    transfers.runningGet = false;
}

void _exit (int code, Socket* toServer) {
    //close put/get thread
    waitPut(&transfers);
    waitGet(&transfers);
    // close main thread
    toServer->close_();
    // exit
    exit(code);
}

void server_input_handler(string recv, Socket *mainSocket, ofstream* file_out) {
    if (recv.substr(0,3) == "get") {
        // get port & size
        deque<string> recvSplit = str_split(recv, " ");
        // finish any running get thread
        waitGet(&transfers);
        // start on new thread
        thread {recv_file_client, pendingGetFile, stoi(recvSplit[2]), stoll(recvSplit[4]), mainSocket}.detach();
    } else if (recv.substr(0,3) == "put") {
        // get port
        deque<string> splitted = str_split(recv, " ");
        // finish any running put thread
        waitPut(&transfers);
        // start on new thread
        thread {send_file_client, pendingPutFile, stoi(splitted[2]), mainSocket}.detach();
    } else {
        if (recv != "") {
            if (file_out) {
                (*file_out) << recv << endl;
            }
            cout << recv << endl;
        }
    }
}


void client_input_handler (string input, Socket* toServer, ofstream* file_out) {
    UNUSED (file_out);
    // Parse
    Command cmd;
    try {
        parseInto(&cmd, input);
    } catch (std::invalid_argument&) {
        cout << "Error : invalid command : " << input << endl;
        return ;
    }
    // maintain data structures
    if (cmd.cmd == "put") {
        pendingPutFile = cmd.arg1;
    } else if (cmd.cmd == "get") {
        pendingGetFile = cmd.arg1;
    }
    // Send command to server
    toServer->str_send(input.c_str());
    // exit if need be
    if (cmd.cmd=="exit") {
        _exit(0, toServer);
    }

    if (cmd.cmd=="logout") {
        waitGet(&transfers);
        waitPut(&transfers);
    }

    // Handle server response
    if (cmd.expects_output) {
        server_input_handler(toServer->str_recv(), toServer, file_out);
    }
    while (toServer->hasData()>0 ) {
        server_input_handler(toServer->str_recv(), toServer, file_out);
    }
}


int main(int argc, char **argv) {
    // Make a short REPL to send commands to the server
    // Make sure to also handle the special cases of a get and put command
    if (argc != 3 && argc != 5) {
        cout << "Error: wrong number of parameters for client";
        exit(1);
    }
    // init socket and connect<
    // bind to port 0 to let the OS choose a random port
    Socket toServer ("127.0.0.1", 0, SOCKET_TIMEOUT);

    // convert port from char* to int
    char *end;
    int port = strtol(argv[2], &end, 10);
    if (*end) {
        cout << "Error: invalid port" << endl;
        exit(1);
    }
    servAddress = argv[1];
    toServer.connect_(servAddress, port);
    bool script_mode = (argc == 5);

    /*
     * Script mode - run until script runs out, then go to interactive mode
     */
    if (script_mode) {
        string line;
        // open input file
        ifstream infile (argv[3]);
        if (! infile.is_open()) {
            string error = "Error: unable to open infile";
            cout << error << endl;
            toServer.str_send(error);
            _exit(1, &toServer);
        }
        // open output file
        ofstream outfile (argv[4]);
        if (! outfile.is_open()) {
            string error = "Error: unable to open outfile";
            cout << error << endl;
            toServer.str_send(error);
            infile.close();
            _exit(1, &toServer);
        }
        // read script
        while ( getline (infile,line) ) {
            if (line.rfind("#", 0) == 0 || line.size() == 0) {
                continue;
            }
            // process line
            client_input_handler(line, &toServer, &outfile);
        }
        infile.close();
        outfile.close();
    }

    /*
     * Interactive mode
     */
    string clientinput = "";
    int ret = 0;
    // init epoll
    int epolltimeout = -1; //ms
    int epollsize = 2;
    int epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd < 0) {
        cout << "System error : epoll add error" << endl;
        return -1;
    }
    // add target file descriptors to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = 0;
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, 0, &event);
    if (ret < 0) {
        string res = "System error : epoll add error";
        cout << res << endl;
        return -1;
    }
    struct epoll_event event2;
    event2.events = EPOLLIN;
    event2.data.fd = toServer.getfd();
    ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, toServer.getfd(), &event2);
    if (ret < 0) {
        cout << "System error : epoll add error" << endl;
        return -1;
    }
    // REPL loop
    while (true) {
        struct epoll_event activityList[epollsize];
        ret = epoll_wait(epollfd, activityList, epollsize, epolltimeout);
        for (auto activity : activityList) {
            if ((activity.events & EPOLLIN)!=0) {
                if (activity.data.fd == 0) {
                    //cout << "stdin activity" << endl;
                    getline(cin, clientinput);
                    client_input_handler(clientinput, &toServer, NULL);
                    //fflush(stdin); // normally not needed
                }
                if (activity.data.fd == toServer.getfd()) {
                    //cout << "server activity" << endl;
                    server_input_handler(toServer.str_recv(), &toServer, NULL);
                }
            }
        }
    }
}
