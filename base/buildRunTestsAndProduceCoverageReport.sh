#!/bin/bash
# assumes this is executed from everest-base/
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
echo "Build with python coverage capability"
python3 -m build_scripts.artisan_fetch build/release-package.xml
./build.sh --python-coverage
echo "build Run Tests and Produce Coverge Report.sh with systemtests: ${SYSTEM_TEST}"