#!/bin/bash

pgrepStacks()
{
    local PIDS=$(pgrep $1)
    for PID in $PIDS
    do
        [ -d "/proc/$PID" ] && printStack $PID
    done
}

printStack()
{
    local PID=$1
    [ -d "/proc/$PID" ] || { pgrepStacks $PID ; return ; }
    echo $PID
    [ -d "/proc/$PID" ] || { echo "Can't find process: $PID" >&2 ; exit 1 ; }
    (
    echo "set pagination 0";
    echo "thread apply all bt";
    echo "quit";
    cat /dev/zero ) | gdb "/proc/$PID/exe" "$PID"
}

for PID in "$@"
do
    printStack "$PID"
done
