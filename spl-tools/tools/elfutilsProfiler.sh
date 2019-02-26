#!/bin/bash

pid=$1
[[ -d /proc/${pid} ]] || pid=$(pgrep -f ${pid})
COUNT=$2
[[ -n ${COUNT} ]] || COUNT=500

[[ -n ${pid} ]] || {
    echo "FAILED: $pid \$pid is empty"
    exit 1
}

for x in $(seq 1 ${COUNT}); do
   eu-stack -p "${pid}" 2> /dev/null
  sleep 0.01
  echo ${x} >&2
done
