#!/usr/bin/env bash

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi


rm -rf "$SCRIPTDIR"
[[ -n $NO_REMOVE_GROUP ]] || delgroup sophos-spl-group 2>/dev/null >/dev/null

