#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
export BASE
cd "${STARTINGDIR}"

BUILD_DIR=/vagrant/sspl-plugin-anti-virus/build64
EXE="$BASE/susi_scanner2"
[[ -f "$EXE" ]] || EXE=${BUILD_DIR}/products/susi_experiment_scanner/susi_scanner2
[[ -f "$EXE" ]] || {
  echo "Failed to find $EXE"
  exit 1
}

file "$EXE"

export PLUGIN_INSTALL=/opt/sophos-spl/plugins/av
export SUSI_DIR=${PLUGIN_INSTALL}/chroot/susi/distribution_version
export LD_LIBRARY_PATH=$BASE:$BASE/lib64:$PLUGIN_INSTALL/lib64:$SUSI_DIR:$SUSI_DIR/version1
ldd "$EXE" || {
  echo "Failed to ldd $EXE"
  exit 2
}

TOOL=
# TOOL="gdb --args"
# TOOL="valgrind --leak-check=full"
TOOL="strace -f -s 128 -o /tmp/strace.log"
echo $TOOL "$EXE" "$@"
exec $TOOL "$EXE" "$@"
