#!/usr/bin/env bash
[[ -n $SOPHOS_INSTALL ]] || SOPHOS_INSTALL="$1"
[[ -n $SOPHOS_INSTALL ]] || SOPHOS_INSTALL="/opt/sophos-spl"

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

cd $SOPHOS_INSTALL
cp -av $ABS_SCRIPTDIR/files/* .
