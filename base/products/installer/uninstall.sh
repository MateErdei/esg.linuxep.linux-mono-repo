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

PATH=$PATH:/usr/sbin:/sbin

USERNAME=sophos-spl-user
GROUPNAME="sophos-spl-group"
if [[ -z $NO_REMOVE_USER ]]
then
    DELUSER=$(which deluser 2>/dev/null)
    USERDEL=$(which userdel 2>/dev/null)

    if [[ -x "$DELUSER" ]]
    then
        "$DELUSER" "$USERNAME" 2>/dev/null >/dev/null
    elif [[ -x "$USERDEL" ]]
    then
        "$USERDEL" "$USERNAME" 2>/dev/null >/dev/null
    else
        echo "Unable to delete user $USERNAME" >&2
    fi

    ## Can't delete the group if we aren't deleting the user
    if [[ -z $NO_REMOVE_GROUP ]]
    then
        GROUP_DELETER=$(which delgroup 2>/dev/null)
        [[ -x "$GROUP_DELETER" ]] || GROUP_DELETER=$(which groupdel 2>/dev/null)
        if [[ -x "$GROUP_DELETER" ]]
        then
            "$GROUP_DELETER" "$GROUPNAME" 2>/dev/null >/dev/null
        else
            echo "Unable to delete group $GROUPNAME" >&2
        fi
    fi
fi
