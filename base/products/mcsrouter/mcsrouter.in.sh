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

pythonzip=$BASE_DIR/lib64/python39.zip
mcsrouterzip=$BASE_DIR/lib64/mcsrouter.zip
pythonExecutable=$BASE_DIR/bin/python3

export PYTHONPATH=@PYTHONPATH@
export PYTHONHOME=$INST_DIR/base/

export LD_LIBRARY_PATH=$INST_DIR/base/lib:$INST_DIR/base/lib64
cd ${INST_DIR}/tmp
exec $pythonExecutable  @PYTHON_ARGS_FOR_PROD@ -m mcsrouter.mcs_router --no-daemon "$@"
