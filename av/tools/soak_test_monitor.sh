#!/bin/bash

set -ex

function record_memory_usage()
{
    ps -C sophos_threat_d --no-headers -o pid,comm,pcpu,cputime,pmem,rss,vsz,size | sed -e 's/^ *//' -e's/  */,/g'
}

LOG=$1
[[ -n $LOG ]] || LOG=/tmp/soak_log.csv
echo "PID,COMM,PCPU,CPUTIME,PMEM,RSS,VSZ,SIZE" >$LOG

record_memory_usage >>$LOG
while sleep 900
do
    record_memory_usage >>$LOG
done

echo "FINISHED"
