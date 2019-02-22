#!/bin/bash

pid=$1
[[ -d /proc/$pid ]] || pid=$(pgrep $pid)

for x in $(seq 1 500); do
   gdb -ex "set pagination 0" -ex "thread apply all bt" -batch -p $pid 2> /dev/null
  sleep 0.01
done
