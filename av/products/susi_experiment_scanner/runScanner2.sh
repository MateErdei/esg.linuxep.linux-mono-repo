#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "${STARTINGDIR}"

export BASE

export GOOD=${GOOD:-good}
export PLUGIN_INSTALL=${BASE}/${GOOD}
export SUSI_DIR=${PLUGIN_INSTALL}/chroot/susi/distribution_version
export LD_LIBRARY_PATH=$BASE:$BASE/lib64:$PLUGIN_INSTALL/lib64:$SUSI_DIR:$SUSI_DIR/version1
ldd "$BASE/susi_scanner2"

TOOL=
# TOOL="gdb --args"
TOOL=valgrind
exec $TOOL "$BASE/susi_scanner2" "$@"
