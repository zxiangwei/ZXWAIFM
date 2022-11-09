#!/bin/bash

MEM_SERVER_SSH_IP=128.110.218.165
MEM_SERVER_SSH_USER=zhj

MEM_SERVER_DPDK_IP=18.18.1.3
MEM_SERVER_PORT=8000
MEM_SERVER_STACK_KB=65536

AIFM_PATH=..
SHENANGO_PATH=../../shenango

function ssh_execute {
    ssh $MEM_SERVER_SSH_USER@$MEM_SERVER_SSH_IP $1
}

function ssh_execute_tty {
    ssh $MEM_SERVER_SSH_USER@$MEM_SERVER_SSH_IP -t $1
}


function say_failed() {
    echo -e "----\e[31mFailed\e[0m"
}

function say_passed() {
    echo -e "----\e[32mPassed\e[0m"
}

function assert_success {
    if [[ $? -ne 0 ]]; then
        say_failed
        exit -1
    fi
}

function kill_process {
    pid=`pgrep $1`
    if [ -n "$pid" ]; then
	{ sudo kill $pid && sudo wait $pid; } 2>/dev/null
    fi
}

function ssh_kill_process {
    pid=`ssh_execute "pgrep $1"`
    if [ -n "$pid" ]; then
	ssh_execute "{ sudo kill $pid && sudo wait $pid; } 2>/dev/null"
    fi
}

function kill_local_iokerneld {
    kill_process iokerneld
}

function run_local_iokerneld {
    kill_local_iokerneld
    sudo $SHENANGO_PATH/iokerneld $@ > /dev/null 2>&1 &
    disown -r
    assert_success
    sleep 3
}

function rerun_local_iokerneld {
    kill_local_iokerneld
    run_local_iokerneld simple
}

function rerun_local_iokerneld_noht {
    kill_local_iokerneld
    run_local_iokerneld simple noht
}

function rerun_local_iokerneld_args {
    kill_local_iokerneld
    run_local_iokerneld $@
}

function kill_mem_server {
    ssh_kill_process iokerneld
    ssh_kill_process tcp_device_serv
}

function run_mem_server {
    ssh_execute "sudo $SHENANGO_PATH/iokerneld simple" > /dev/null 2>&1 &
    sleep 3
    ssh_execute_tty "sudo sh -c 'ulimit -s $MEM_SERVER_STACK_KB; \
                     tcp_device_server $AIFM_PATH/configs/server.config \
                     $MEM_SERVER_PORT'" > /dev/null 2>&1 &
    sleep 3
}

function rerun_mem_server {
    kill_mem_server
    run_mem_server
}

function run_program {
    sudo stdbuf -o0 sh -c "$1 $AIFM_PATH/configs/client.config \
                           $MEM_SERVER_DPDK_IP:$MEM_SERVER_PORT"
}

function run_program_noht {
    sudo stdbuf -o0 sh -c "$1 $AIFM_PATH/configs/client_noht.config \
                           $MEM_SERVER_DPDK_IP:$MEM_SERVER_PORT"
}

function run_single_test {
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
}

run_single_test $1
