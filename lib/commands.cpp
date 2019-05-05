#include "commands.hpp"
using namespace std;


/* the login command starts authentication. The username must be one of the allowed usernames in the configuration file.
 */
string login (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    //_print_session (usersmap);
    process_username(cmd->arg1.c_str(), cmd->arg1.length());
    // Reset any previous login attempt
    _stop_pending_auth(usersmap, clientFd);
    // First, check another user is not already logged in
    if (_is_authenticated(usersmap, clientFd)) {
        string error = "Error : User " + cmd->arg1 + " is already logged in";
        cout << error << endl;
        return error;
    }
    // Look for username in "not-logged-in" users
    string uname = cmd->arg1;
    bool found = false;
    long unsigned int i = 0;
    for (i=0; i<(*usersmap).at(-1).size() && (!found); i++) {
        if ( 0==uname.compare(((*usersmap).at(-1)[i]).uname) ) {
            found = true;
            i = i-1;
        }
    }
    if (!found) {
        string error = "Error: user '"+cmd->arg1+"' doesn't exist";
        cout << error << endl;
        return error;
    }
    // obtain user data
    User user = move((*usersmap).at(-1)[i]);
    user.pending_login = true;
    user.fd = clientFd;
    // erase old user state
    usersmap->at(-1).erase(usersmap->at(-1).begin()+i);
    // insert new user data
    (*usersmap)[clientFd] = vector<User>();
    (*usersmap).at(clientFd).push_back(user);
    // no output for this command
    return "";
}


/* the pass command must directly follow the login command. The password must match the password for the earlier
 * specified user.
 */
string pass (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    try {
        usersmap->at(clientFd);
    } catch (const std::out_of_range&) {
        string error = "Error: call 'login' before pass";
        cout << error << endl;
        return error;
    }

    if ((*usersmap).at(clientFd).size() != 1) {
        string error = "Error : Several clients connected via the same file descriptor";
        cout << error << endl;
        return error;
    }

    User* user = &((*usersmap).at(clientFd)[0]);

    string pass = cmd->arg1;
    if (user->logged_in) {
        string error = "Error : User is already logged in";
        _stop_pending_auth(usersmap, clientFd);
        cout << error << endl;
        return error;
    }
    if (!user->pending_login) {
        string error = "Error : User isn't in pending login";
        _stop_pending_auth(usersmap, clientFd);
        cout << error << endl;
        return error;
    }
    if (user->pass.compare(pass) != 0) {
        string error = "Error : Wrong password";
        _stop_pending_auth(usersmap, clientFd);
        cout << error << endl;
        return error;
    }
    user->logged_in = true;
    user->pending_login = false;
    // no output for this command
    return "";
}


/* ping may always be executed even if the user is not authenticated and
 * returns the output of the Unix command ping $HOST -c 1
 */
string ping (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    vector<string> piecewise_cmd = {cmd->cmd, cmd->arg1, "-c 1"};
    return _check_cmd(str_join(piecewise_cmd));
}


/* ls command may only be executed after a successful authentication and lists
 * the available files in the current working directory as reported by ls -l
 */
string ls (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    // the ls command may only be executed after a successful authentication
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    // executed command only if client is authenticated
    vector<string> piecewise_cmd = {"ls", "-l", basedir+string(pwd)};
    string tmp = _check_cmd(str_join(piecewise_cmd));
    return tmp;
}


/* Helper for cd
 */
string _cd_relative (const string& basedir, deque<string> pwd_dq, deque<string> cdstr) {
    if (cdstr.size() == 0) {
        return str_join_q(pwd_dq, "/");
    }
    string first = cdstr.front();
    cdstr.pop_front();
    if ( first=="." || first=="" ) {
        // Nothing to do
    } else if (first == "..") {
        // purge unwanted empty elements
        while (pwd_dq.size()>0 && pwd_dq.back()=="") {
            pwd_dq.pop_back();
        }
        if (pwd_dq.size() == 0) {  // we reached './', can't go any higher
            throw invalid_argument("directory out of bounds");
        }
        pwd_dq.pop_back();
    } else {
        // update pwd_dq first, check validity second
        pwd_dq.emplace_back(first);
        if ( !_dir_exists (str_join_q(pwd_dq, "/"), basedir) ) {
            throw runtime_error("directory does not exist : "+basedir+str_join_q(pwd_dq, "/"));
        }
    }
    return _cd_relative (basedir, pwd_dq, cdstr);
}

/* the cd command may only be executed after a successful authentication and
 * changes the current working directory to the specified one
 */
string cd (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    if (cmd->arg1.length()>MAX_PATH_LEN) {
        string error = "Error: the path is too long.";
        cout << error << endl;
        return error;
    }
    string newpwd = "";
    deque<string> pwd_dq;
    if (cmd->arg1[0] == '/') { // handle absolute paths
        pwd_dq = deque<string> {""};
    } else {
        pwd_dq = str_split(string(pwd), "/");
    }
    deque<string> cdcmd = str_split(string(cmd->arg1), "/");
    try {
        newpwd = _cd_relative (basedir, pwd_dq, cdcmd);
        if (newpwd.length()>MAX_PATH_LEN) {
            string error = "Error: the path is too long.";
            cout << error << endl;
            return error;
        }
        strncpy(pwd, newpwd.c_str(), MAX_PATH_LEN);
    } catch (const std::invalid_argument& e) {
        string error = "Error: invalid path";
        cout << error << endl;
        return error;
    } catch (const std::runtime_error& e) {
        string error = "Error: directory does not exist";
        cout << error << endl;
        return error;
    }

    // no output for this command
    return "";
}


/* mkdir may only be executed after a successful authentication and creates a
 * new directory with the specified name in the current working directory. If a
 * file or directory with the specified name already exists this command returns
 * an error.
 */
string mkdir (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    // check if a file or directory with the specified name already exists in
    // the current working directory
    string newdir = string(pwd)+cmd->arg1;
    struct stat buffer;
    if (stat((basedir+newdir).c_str(), &buffer) == 0) {
        string error = "Error: file or directory with specified name already exists.";
        cout << error << endl;
        return error;
    }
    if (!_sane_filename(cmd->arg1)) {  // sanitize input == just check we don't move from pwd
        string error = "Error : invalid directory name";
        cout << error << endl;
        return error;
    }
    if (newdir.length()>MAX_PATH_LEN) {  // sanitize input == just check we don't move from pwd
        string error = "Error : path is too long";
        cout << error << endl;
        return error;
    }
    if (_dir_exists(basedir+newdir)) {
        string error = "Error : mkdir: cannot create directory "+newdir+": File exists";
        cout << error << endl;
        return error;
    }

    int ret = mkdir((basedir+newdir).c_str(), 0777);
    if (ret != 0) {
        return string("Error: impossible to create the folder! (")+strerror(errno)+") ";
    }

    return "";
}


/* the rm command may only be executed after a successful authentication and
 * deletes the file or directory with the specified name in the current working
 * directory
 */
string rm (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    // add the -r flag because we should remove also directories
    if (!_sane_filename(cmd->arg1)) { // sanitize input == just check we don't move from pwd
        string error = "Error : Invalid path";
        cout << error << endl;
        return error;
    }
    // rm will check if file exist himself
    vector<string> piecewise_cmd = {"rm -r", basedir+pwd+string(cmd->arg1)};
    string ret = _check_cmd(str_join(piecewise_cmd));
    if (ret.find("No such file or directory") != string::npos) {
        return "Error : no such file or directory";
    }
    if (ret.find("refusing to remove") != string::npos) {
        return "Error : invalid path";
    }

    return ret;
}


/* The get command may only be executed after a successful authentication.  The
 * get command takes exactly one parameter (get $FILENAME) and retrieves a file
 * from the current working directory. The server responds to this command with
 * a TCP port and the file size (in ASCII decimal) in the following format: get
 * port: $PORT size: $FILESIZE (followed by a newline) where the client can
 * connect to retrieve the file. In this instance, the server will spawn a
 * thread to send the file to the clients receiving thread.  The server may
 * only leave one port open per client. Note that client and server must handle
 * failure conditions, e.g., if the client issues another get or put request,
 * the server will only handle the new request and ignore (or drop) any stale
 * ones.
 */
string get (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    //Check we have access to the file (ie not in a parent/child directory
    if (!_sane_filename(cmd->arg1)) {
        string error = "Error: access denied!";
        cout << error << endl;
        return error;
    }
    //Check total length of path
    if ((pwd+cmd->arg1).length() > MAX_PATH_LEN) {
        string error = "Error: the path is too long.";
        cout << error << endl;
        return error;
    }
    string path_to_file = string(basedir + string(pwd) + cmd->arg1);
    //Check file exist is current dir and that it is a file
    long long filesize = _file_size (path_to_file);
    if (filesize<0) {
        string error = "Error: File not found";
        cout << error << endl;
        return error;
    }
    int port = 10000+clientFd; //Every client will have one  port for him.

    waitGet(trans);

    thread{send_file_server, path_to_file, port, trans, clientSock}.detach();
    return "get port: "+to_string(port)+" size: "+ to_string(filesize);
}


/* The put command may only be executed after a successful authentication.  The
 * put command takes exactly two parameters (put $FILENAME $SIZE) and sends the
 * specified file from the current local working directory (i.e., where the
 * client was started) to the server.  The server responds to this command with
 * a TCP port (in ASCII decimal) in the following format: put port: $PORT. In
 * this instance, the server will spawn a thread to receive the file from the
 * clients sending thread.
 */
string put (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    if (!_sane_filename(cmd->arg1)) {
        string error = "Error: access denied!";
        cout << error << endl;
        return error;
    }
    //Check total length of path
    string path_to_file = basedir+pwd+cmd->arg1;
    if ((pwd+cmd->arg1).length() > MAX_PATH_LEN) {
        string error = "Error: the path is too long.";
        cout << error << endl;
        return error;
    }
    //Check file exist is current dir and that it is a file
    long long filesize = _file_size (path_to_file);
    if (filesize>0) {
        string error = "Error: File already exists";
        cout << error << endl;
        return error;
    }
    int port = 20000+clientFd;

    waitPut(trans);

    thread{recv_file_server, path_to_file, stoll(cmd->arg2), port, trans, clientSock}.detach();

    return "put port: "+to_string(port);
}


/* The grep command may only be executed after a successful authentication. The
 * command takes exactly one paramter, i.e. the pattern, and searches every
 * file in the current directory and its subdirectories for the requested
 * pattern. The pattern follows the Regular Expressions rules. The server
 * responds to this command with a line separated list of addresses for
 * matching files.
 */
string grep (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    vector<string> piecewise_cmd = {"grep -Rl", string(cmd->arg1), basedir+pwd};
    return _check_cmd(str_join(piecewise_cmd));
}


/* The date dommand may only be executed after a successful authentication and
 * returns the output from the Unix date command.
 */
string date (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    return _check_cmd("date");
}


/* May only be executed after a successful authentication and returns the name
 * of the currently logged user.
 */
string whoami (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    string iam = usersmap->at(clientFd)[0].uname;
    cout << iam << endl;
    return iam;
}


/* The w command may only be executed after a successful authentication and
 * returns a list of each logged in user on a single line space separated.
 */
string w (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    UNUSED(trans);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }

    string names = "";
    // traverse data structure and retrieve names of logged in users
    for (auto const &entry: *usersmap) {
        if (entry.first != -1) {
            // only a fixed fd
            names += entry.second[0].uname + " ";
        }

    }

    // we are sure that there is at least one logged in user because of the
    // initial _is_authenticated
    cout << names << endl;
    return names;
}


/* May only be executed after a successful authentication and logs the user out
 * of the sessions.
 */
string logout (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (!_is_authenticated(usersmap, clientFd)) {
        return _error_authenticated(cmd->cmd);
    }
    waitGet(trans);
    waitPut(trans);
    // retrieve and update client structure
    User u = move(usersmap->at(clientFd)[0]);
    u.logged_in=false;
    u.pending_login=false;
    u.fd=-1;
    // update session
    usersmap->erase(clientFd);
    usersmap->at(-1).push_back(u);
    return "";
}


/* The exit command can always be executed and signals the end of the command
 * session.
 */
string exit (const string& basedir, char *pwd, Session* usersmap, Socket* clientSock, Command* cmd, Transfers* trans) {
    UNUSED(basedir);
    UNUSED(pwd);
    int clientFd = clientSock->getfd();
    _stop_pending_auth(usersmap, clientFd);
    if (_is_authenticated(usersmap, clientFd)) {
        // Disconnect user
        logout(basedir,pwd, usersmap, clientSock, cmd, trans);
    }
    return "";
}
