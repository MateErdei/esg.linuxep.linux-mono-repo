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

function create_symlinks()
{
    local F=$1
    local SHORTER=${F%.*}
    local SO_CUT=${SHORTER%.so}

    if [[ ! -e $SHORTER ]]
    then
        ln -snfv $(basename $F)  $SHORTER
    fi

    [[ $SO_CUT == $SHORTER ]] && create_symlinks $SHORTER
}

for F in $(find "${SUSI_DIR}" -name '*.so.*' -type f)
do
    create_symlinks "$F"
done

TOOL=
# TOOL="gdb --args"
# TOOL="valgrind --leak-check=full"
exec $TOOL "$BASE/susi_scanner2" "$@"
