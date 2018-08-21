#!/bin/sh

# Sets LD_LIBRARY_PATH to current imports

# source ./setLD_LIBRARY_PATH.sh
B=$(pwd)
INPUT=$B/redist
[[ -d $INPUT ]] || INPUT=/redist/binaries/linux11/input

export LD_LIBRARY_PATH=$INPUT/boost/lib64:$INPUT/SUL/lib64:$INPUT/curl/lib64

