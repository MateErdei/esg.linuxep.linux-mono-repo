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

pythonzip=$BASE_DIR/lib64/python39.zip
mcsrouterzip=$BASE_DIR/lib64/mcsrouter.zip
pythonExecutable=@PRODUCT_PYTHON_EXECUTABLE@

export PYTHONPATH=@PYTHONPATH@
export PYTHONHOME=@PRODUCT_PYTHONHOME@
export LD_LIBRARY_PATH=@PRODUCT_PYTHON_LD_LIBRARY_PATH@

cd ${BASE_DIR}/../tmp
exec $pythonExecutable  @PYTHON_ARGS_FOR_PROD@  -m mcsrouter.register_central  "$@"
