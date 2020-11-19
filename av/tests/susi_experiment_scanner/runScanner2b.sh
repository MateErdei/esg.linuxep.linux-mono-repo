#!/bin/bash

failure()
{
    local CODE=$1
    shift
    echo "$@" >&2
    exit $CODE
}

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
export BASE
cd ..
PRODUCTS=$(pwd)
cd ..
SOURCE_DIR=$(pwd)
BUILD_DIR=${SOURCE_DIR}/build64
[[ -d ${BUILD_DIR} ]] || BUILD_DIR=/vagrant/sspl-plugin-anti-virus/build64

cd "${STARTINGDIR}"

EXE="$BASE/susi_scanner2"
[[ -f "$EXE" ]] || EXE=${BUILD_DIR}/products/susi_experiment_scanner/susi_scanner2
[[ -f "$EXE" ]] || failure 1 "Failed to find $EXE"

file "$EXE"

export PLUGIN_INSTALL=/opt/sophos-spl/plugins/av
export SUSI_DIR=${PLUGIN_INSTALL}/chroot/susi/distribution_version
export LD_LIBRARY_PATH=$BASE:$BASE/lib64:$PLUGIN_INSTALL/lib64:$SUSI_DIR:$SUSI_DIR/version1
ldd "$EXE" || failure 2 "Failed to ldd $EXE"

TOOL=
# TOOL="gdb --args"
# TOOL="valgrind --leak-check=full"
TOOL="strace -f -s 128 -o /tmp/strace.log"
echo $TOOL "$EXE" "$@"
exec $TOOL "$EXE" "$@"
