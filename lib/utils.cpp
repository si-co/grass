//
// Created by quentin on 22/04/2019.
//

#include "utils.hpp"
using namespace std;


//====================================================================================================================//
//==========================================    String  helpers    ===================================================//
//====================================================================================================================//

// string.split() to double-ended queue
deque<string> str_split (const string& input, const string& delimiter) {
    deque<string> res;
    size_t start = 0;
    size_t stop= 0;
    while ((stop = input.find(delimiter, start)) != std::string::npos) {
        res.push_back(input.substr(start, stop - start));
        start = stop+1;
    }
    res.push_back(input.substr(start, stop - start));
    return res;
}

// string.join() from array
string str_join (vector<string> piecewise_str) {
    stringstream tmp_stream;
    copy(piecewise_str.begin(), piecewise_str.end(), ostream_iterator<string>(tmp_stream, " "));
    string concatenated_str = tmp_stream.str();
    // remove trailing space
    if (!concatenated_str.empty()) {
        concatenated_str.resize(concatenated_str.length() - 1);
    }
    return concatenated_str;
}

// string.join from double-ended queue
string str_join_q (deque<string> inputs, const string& delimiter) {
    stringstream acc;
    for (string& s : inputs) {
        if (s != "") {
            acc << s << delimiter;
        }
    }
    // __don't__ remove trailing delimiter
    return acc.str();
}



//====================================================================================================================//
//======================================    Execution primitives    ==================================================//
//====================================================================================================================//

/*  Execute an arbitrary Unix command;
 */
string _check_cmd(const string& cmd) {
    string result = ""; // Commands like mkdir may not ouptut anything
    array<char, CMD_OUTPUT_MAX_LEN> buf;
    unique_ptr<FILE, decltype(&pclose)> pipe (popen((cmd+" 2>&1").c_str(), "r"), pclose);
    if (!pipe) {
        string error = "Error: "+ cmd + " : popen() failed!";
        cout << error << endl;
        return error;
    }
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) {
        result += buf.data();
    }
    // server output
    cout << result;
    return result;
}


/* Send a file to the client on separate thread (for 'get')
 */
void send_file_server(const string& path, int port, Transfers* trans, Socket* mainSocket) {
    trans->runningGet = true;
    Socket handshakeSocket {"127.0.0.1", port};
    Socket fileSocket = handshakeSocket.awaitClient();
    handshakeSocket.close_();


    string error = send_file_from(fileSocket, path, trans, true);
    if (error != "") {
        cout << error << endl;
        mainSocket->str_send(error);
    }
    fileSocket.close_();
    trans->runningGet = false;
}


// this is separate because re-used in client
// in serv: Get
// in client put
string send_file_from(Socket& sock, const string& fileName, Transfers* trans, bool serv) {

    //open file in binary mode, get pointer at the end of the file (ios::ate)
    ifstream myfile (fileName, ios::in|ios::binary|ios::ate);

    if (myfile.is_open()) {
        //position get pointer at the begining of the file
        myfile.seekg(0, ios::beg);


        // initialize the buffer
        char buffer[FILE_CHUNK_SIZE];
        while (!myfile.eof() && !(serv && trans->interruptGet) && !(!serv && trans->interruptPut)) {
            // read myfile into buffer
            myfile.read(buffer, FILE_CHUNK_SIZE);
            ssize_t readsize = myfile.gcount();
            // send file using socket
            sock.send_(buffer, readsize);
        }
        if (serv && trans->interruptGet) {
            myfile.close();
            trans->interruptGet = false;
            return "";
        }
        if (!serv && trans->interruptPut) {
            myfile.close();
            trans->interruptPut = false;
            return "";
        }

        //sock.str_send(EOF+"");    // TODO (done?) : closing the socket should do the job
        myfile.close();
        return "";
    } else {
        return "Error: can't open file "+fileName;
    }
}


/* Receive file from client on separate thread (for 'put')
 */
void recv_file_server(const string& path, long long size, int port, Transfers* trans, Socket* mainSocket) {
    trans->runningPut = true;
    Socket handshakeSocket {"127.0.0.1", port};
    Socket fileSocket = handshakeSocket.awaitClient();
    handshakeSocket.close_();

    char buffer[FILE_CHUNK_SIZE];
    string result = recv_file_from(fileSocket, buffer, path, size, trans, true);
    if (result != "") {
        cout << ">>>" << result << endl;
        mainSocket->str_send(result);
    }
    fileSocket.close_();
    trans->runningPut = false;
}


// this is separate because re-used in client
// In serv: Put
// In client: get
string recv_file_from(Socket& sock, char* buffer, const string& fileName, long long size, Transfers* trans, bool serv) {
    char buffer_file[size];
    long long size2 = size;
    long long current_size = 0;

    ssize_t readsize = 0;
    while (!(serv && trans->interruptPut) && !(!serv && trans->interruptGet) && (0 != (readsize = sock.recv_(buffer, FILE_CHUNK_SIZE)))) {
        //check total size
        memcpy(buffer_file+current_size, buffer, readsize);
        current_size += readsize;
    }

    if ((serv && trans->interruptPut)) {
        trans->interruptPut = false;
        return ""; //Not an error, we just have a more recent put/get commnd
    }

    if ((!serv && trans->interruptGet)) {
        trans->interruptGet = false;
        return ""; //Not an error, we just have a more recent put/get commnd
    }
    if (current_size != size2) {
        return "Error: file transfer failed : expected : " + to_string(size2) + ", got : " +
        to_string(current_size);
    }

    // It is a sucess => write in file
    ofstream file(fileName);
    if (!file.is_open()) {
        cout << "file not open" << endl;
        return "";
    }
    file.write(buffer_file, size2);
    file.close();

    return "";
}


void waitPut(Transfers* trans) {
    if (trans->runningPut) {
        trans->interruptPut = true;
        // interruptFile is shared between threads
        while (trans->interruptPut && trans->runningPut);
    }
}

void waitGet(Transfers* trans) {
    if (trans->runningGet) {
        trans->interruptGet = true;
        // interruptFile is shared between threads
        while (trans->interruptGet && trans->runningGet);
    }
}


//====================================================================================================================//
//=========================================    Paths helpers    ===================================================//
//====================================================================================================================//

/*  String sanitizer helpers
 */
inline bool _no_illegal_chars (string input, string illegalChars) {
    for (char& c : illegalChars) {
        bool found = (input.find(c) != string::npos);
        if(found) {
            return false;
        }
    }
    return true;
}

bool _sane_filename (const string& s) {
    return _no_illegal_chars (s, "/");
}

/*  Check wether path points to a directory in the filesystem. Can accept a
 *  basedir.
 */
bool _dir_exists (const string& path, const string& basedir) {
    // cf https://linux.die.net/man/2/stat
    struct stat res;
    string fullpath = (basedir+path);
    const char* s = fullpath.c_str();
    if (stat(s, &res) == -1) {
        int err = errno;
        switch (err) {
        case ENOENT  :
            return false;
        case ENOTDIR :
            return false;
        case EACCES       :
            break;
        case EFAULT       :
            break;
        case ENAMETOOLONG :
            break;
        default           :
            break;
        }
    }
    switch (res.st_mode & S_IFMT) {
    case S_IFDIR:
        return true;     // directory
    case S_IFREG:
        return false;    // regular file
    case S_IFSOCK:
        return false;    // socket
    case S_IFLNK:
        return false;    // symlink
    case S_IFBLK:
        return false;    // block device
    case S_IFCHR:
        return false;    // character device (?)
    case S_IFIFO:
        return false;    // fifo / pipe (?)
    default:
        throw runtime_error ("Unknown / invalid stat struct for : "+basedir+", "+path);
    }
}

/*  Check wether path points to a regular file in the filesystem. Can accept a
 *  basedir.
 */
bool _file_exists (const string& path, const string& basedir) {
    // cf https://linux.die.net/man/2/stat
    struct stat res;
    const char* s = (basedir+path).c_str();
    if (stat(s, &res) == -1) {
        int err = errno;
        switch (err) {
        case ENOENT  :
            return false;
        case ENOTDIR :
            return false;
        case EACCES       :
            break;
        case EFAULT       :
            break;
        case ENAMETOOLONG :
            break;
        default           :
            throw runtime_error ("Failed to stat file : "+basedir+", "+path);
            break;
        }
    }
    switch (res.st_mode & S_IFMT) {
    case S_IFREG:
        return true;     // regular file
    case S_IFDIR:
        return false;    // directory
    case S_IFSOCK:
        return false;    // socket
    case S_IFLNK:
        return false;    // symlink
    case S_IFBLK:
        return false;    // block device
    case S_IFCHR:
        return false;    // character device (?)
    case S_IFIFO:
        return false;    // fifo / pipe (?)
    default:
        throw runtime_error ("Unknown / invalid stat struct for : "+basedir+", "+path);
    }
}


/*  Check wether path points to a regular file in the filesystem. Can accept a
 *  basedir.
 */
long long _file_size (const string& path, const string& basedir) {
    // cf https://linux.die.net/man/2/stat
    struct stat res;
    string target = string(basedir+path);
    const char* s = target.c_str();
    if (stat(s, &res) == -1) {
        int err = errno;
        switch (err) {
        case ENOENT       :
            return -1LL;
        case ENOTDIR      :
            return -1LL;
        case EACCES       :
            return -1LL;
        case EFAULT       :
            return -1LL;
        case ENAMETOOLONG :
            return -1LL;
        default           :
            throw runtime_error ("Failed to stat file : "+basedir+", "+path);
            return -1;
        }
    }
    switch (res.st_mode & S_IFMT) {
    case S_IFREG:
        return res.st_size;     // regular file
    case S_IFDIR:
        return res.st_size;    // directory
    case S_IFSOCK:
        return -1LL;    // socket
    case S_IFLNK:
        return -1LL;    // symlink
    case S_IFBLK:
        return -1LL;    // block device
    case S_IFCHR:
        return -1LL;    // character device (?)
    case S_IFIFO:
        return -1LL;    // fifo / pipe (?)
    default:
        throw runtime_error ("Unknown / invalid stat struct for : "+basedir+", "+path);
    }
}



//====================================================================================================================//
//=====================================    Data structures helpers    ================================================//
//====================================================================================================================//

/*  Check wether user is authenticated
 */
bool _is_authenticated(Session* usersmap, int clientFd) {
    try {
        if (usersmap->at(clientFd).size() != 1) {
            throw std::runtime_error("Illegal state : Several clients connected via the same file descriptor");
        }
        return usersmap->at(clientFd)[0].logged_in;
    } catch (const std::out_of_range&) {
        return false;
    }
}

void process_username(const char* username, size_t num) {
    char buff[20];
    memcpy(buff, username, num);
    //fprintf(stderr, "%s", buff);
}

void _stop_pending_auth(Session* usersmap, int clientFd) {
    try {
        User u = move(usersmap->at(clientFd)[0]);;

        if (u.pending_login) {
            u.pending_login = false;
            u.fd = -1;
            // update session
            usersmap->erase(clientFd);
            usersmap->at(-1).push_back(u);
        }
    } catch (const std::out_of_range&) {
        //Not logged in, nothing to do
    }
}

// generate atuhentication error message
string _error_authenticated(const string& cmd) {
    string error = "Error: " + cmd + " may only be executed after a successful authentication";
    cout << error << endl;
    return error;
}


// print map<int, vector<user>>, for debug
void _print_session (Session* s) {
    cout << "[session]    SESSION" << endl;
    for (pair<int, vector<User>> p : *s) {
        cout << "[session]  " << p.first << " --> ";
        if (p.second.size() > 0) {
            User *u = &(p.second[0]);
            cout << "0: " << u->uname << ", " << u->pass << ", " << u->fd << ", " << u->pending_login << ", " << u->logged_in << endl;
            if (p.second.size() > 1) {
                for (long unsigned int i = 1; i < p.second.size(); i++) {
                    User *u = &(p.second[i]);
                    cout << "[session]         " << i << ": " << u->uname << ", " << u->pass << ", " << u->fd << ", " << u->pending_login
                         << ", " << u->logged_in
                         << endl;
                }
            }
        } else {
            cout << endl;
        }
    }
}
