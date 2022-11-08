function run_single_test {
    echo "Running test $1..."
    rerun_local_iokerneld
    if [[ $1 == *"tcp"* ]]; then
    	rerun_mem_server
    fi
    if run_program ./bin/$1 2>/dev/null | grep -q "Passed"; then
        say_passed
    else
        say_failed
    fi
}
