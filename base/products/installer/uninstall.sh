#!/usr/bin/env bash

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
BASEDIR="${ABS_SCRIPTDIR%/*}"
INSTDIR="${BASEDIR%/*}"


rm -rf "$INSTDIR"
[[ -n $NO_REMOVE_USER ]] || deluser sophos-spl-user 2>/dev/null >/dev/null
[[ -n $NO_REMOVE_GROUP ]] || delgroup sophos-spl-group 2>/dev/null >/dev/null
