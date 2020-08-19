#!/bin/bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "${STARTINGDIR}"

export SOPHOS_INSTALL=/opt/sophos-spl
export PLUGIN_INSTALL=${SOPHOS_INSTALL}/plugins/av
export SUSI_DIR=${PLUGIN_INSTALL}/chroot/susi/distribution_version

export LD_LIBRARY_PATH=$PLUGIN_INSTALL/lib64:$SUSI_DIR:$SUSI_DIR/version1

exec $BASE/susi_scanner2 "$@"
