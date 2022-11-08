#!/bin/bash

source ../shared.sh

echo "Running test $1..."
rerun_local_iokerneld
if [[ $1 == *"tcp"* ]]; then
  rerun_mem_server
fi
if run_program ./bin/$1 2>/dev/null | grep -q "Passed"; then
    say_passed
    exit 0
else
    say_failed
    exit 1
fi
