#!/usr/bin/env bash


STARTINGDIR=$(pwd)
SCRIPTDIR=${0%/*}
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR=${STARTINGDIR}
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

BASE_DIR=${ABS_SCRIPTDIR%/*}
export BASE_DIR

INST_DIR=${BASE_DIR%/*}
export INST_DIR

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=$INST_DIR
export SOPHOS_INSTALL

BIN_DIR=${BASE_DIR}/bin

pythonzip=$BASE_DIR/lib64/python27.zip
mcsrouterzip=$BASE_DIR/lib64/mcsrouter.zip
pythonExecutable=$BASE_DIR/bin/python

export PYTHONPATH=$pythonzip:$mcsrouterzip
export PYTHONHOME=$INST_DIR/base/

LD_LIBRARY_PATH=$INSTDIR/base/lib:$INSTDIR/base/lib64
export LD_LIBRARY_PATH

exec $pythonExecutable -m mcsrouter.mcs_router --no-daemon "$@"
