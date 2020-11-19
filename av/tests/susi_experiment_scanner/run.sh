#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "${STARTINGDIR}"

export SRC_DIR=~/gitrepos/sspl-tools/sspl-plugin-modular-anti-virus
export REDIST=$SRC_DIR/redist

export SUSI_DIR=$REDIST/susi


export LD_LIBRARY_PATH=$SUSI_DIR:$SUSI_DIR/lib

exec $SRC_DIR/cmake-build-debug/products/susi_experiment_scanner/susi_experiment_scanner $SUSI_DIR "$@"
