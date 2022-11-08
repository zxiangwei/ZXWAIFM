#!/bin/bash

source shared.sh

echo "Running test $1..."
rerun_local_iokerneld
if [[ $1 == *"tcp"* ]]; then
  rerun_mem_server
fi
if run_program ./bin/$1 2>/dev/null | grep -q "Passed"; then
    say_passed
    return 0
else
    say_failed
    return 1
fi
