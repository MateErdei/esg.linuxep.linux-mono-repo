#!/bin/bash
STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
SYSTEM_TEST=""
while [[ $# -ge 1 ]]
do
    case $1 in
        --system-test)
          shift
          SYSTEM_TEST="$1"
          ;;
    esac
    shift
done

echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"