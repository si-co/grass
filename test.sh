#!/bin/bash

# diff command
diff="diff -a"

# define folder for tests
__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
test_folder="$__dir/test"
rundir="$__dir/bin/run"

# clean previous runs
printf "Clearing previous results... "
rm "$test_folder"/*.out > /dev/null 2>&1
rm "$test_folder"/*.stdout.client > /dev/null 2>&1
rm "$test_folder"/*.stdout.server > /dev/null 2>&1
rm -r $rundir > /dev/null 2>&1
printf "Done\n"

# recompile 
printf "\n\nRecompiling\n"
make all;
printf "Done\n\n"

# work in bin
cd bin || exit 1


# function to execute a single test, input is the name of the test
function execute_single_test() {
    
    # variables
    base_filename="$test_folder"/"$1"
    infile="$base_filename".in
    outfile="$base_filename".out
    expected_outfile="$base_filename".expected
    server_output="$base_filename".stdout.server
    client_output="$base_filename".stdout.client

    # test files existence
    if [ ! -f "$infile" ]; then
        echo "Error: specified infile doesn't exist"
        return 
    fi 

    if [ ! -f "$expected_outfile" ]; then
        echo "Error: specified expected_outfile doesn't exist"
        return 
    fi
    
    # clear run directory
    # SC2115: Use "${var:?}" to ensure this never expands to /* .
    rm -rf "${rundir:-bin/run}" > /dev/null 2>&1

    # start test procedure
    printf "    Testing %s... " "$1"

    # execute server in background
    ./server > "$server_output" 2>&1 &
    server_pid=$!
    sleep 0.3

    # execute client with correct parameters
    # note: no need to run client in background: exit when list of commands is
    # terminated
    ./client "127.0.0.1" 4444 "$infile" "$outfile" 2>&1 > "$client_output";
    sleep 1s

    # kill client and server
    # The message isn't coming from either kill or the background command, it's
    # coming from bash when it discovers that one of its background jobs has been
    # killed. To avoid the message, use disown to remove it from bash's job control
    # (see
    # https://stackoverflow.com/questions/8074904/how-to-shield-the-kill-output)
    disown $server_pid
    kill -9 $server_pid > /dev/null 2>&1

    # clean expected_outfile & outfile from comments & empty lines
    cat "$expected_outfile" | sed '/^$/d' | sed '/^#.*$/d' > tmp
    cat "$outfile"          | sed '/^$/d' | sed '/^#.*$/d' > tmp2
    #grep -v "^#" "$expected_outfile" > tmp

    # execute diff
    $diff tmp2 tmp > /dev/null 2>&1
    diff_exit_code=$?

    # check exit code from diff
    if [ $diff_exit_code != 0 ]; then
        printf "\n\ndiff failed with error code %d\n" "$diff_exit_code"
        printf "\n--> Expected outfile:\n\n"
        #cat "$expected_outfile"
        cat tmp
        printf "\n --> Actual Outfile:\n\n"
        #cat "$outfile"
        cat tmp2
        #printf "\n\n\n--> Server output:\n\n"
        #cat "$server_output"
        printf "\n\n"
    else
        # clean files if test works
        rm "$outfile" "$server_output" "$client_output"
        printf "TEST OK!\n"
    fi

    # rm tmp file
    rm tmp2
    rm tmp

    # pause after test
    sleep 1
}


# run all tests
    #WORKSexecute_single_test "login2"
#execute_single_test "mkdir_cd_rm"
execute_single_test "check_cmd_should_capture_stdout"
    #WORKSexecute_single_test "login"
    #WORKSexecute_single_test "mkdir"
    #WORKSexecute_single_test "pass_without_login"

# Clean exit with status 0
exit 0
