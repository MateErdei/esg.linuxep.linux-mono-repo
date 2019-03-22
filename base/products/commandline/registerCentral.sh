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

BIN_DIR=${BASE_DIR}/bin

mcsrouterzip=$BASE_DIR/lib64/mcsrouter.zip

export PYTHONPATH=$mcsrouterzip

exec python -m mcsrouter.register_central "$@"
