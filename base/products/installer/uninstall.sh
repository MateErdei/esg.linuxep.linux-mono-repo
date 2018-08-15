#!/usr/bin/env bash

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
BASEDIR="${ABS_SCRIPTDIR%/*}"
SOPHOS_INSTALL="${BASEDIR%/*}"

export SOPHOS_FULL_PRODUCT_UNINSTALL=1

function removeUpdaterSystemdService()
{
    systemctl stop sophos-spl-update.service
    rm -f "/lib/systemd/system/sophos-spl-update.service"
    rm -f "/usr/lib/systemd/system/sophos-spl-update.service"
    systemctl daemon-reload
}

function removeWatchdogSystemdService()
{
    systemctl stop sophos-spl.service
    rm -f "/lib/systemd/system/sophos-spl.service"
    rm -f "/usr/lib/systemd/system/sophos-spl.service"
    systemctl daemon-reload
}

removeUpdaterSystemdService
removeWatchdogSystemdService

for UNINSTALLER in "$SOPHOS_INSTALL/base/update/var/installedproducts/"*
do
    bash $UNINSTALLER || echo "Failed to uninstall $(basename $UNINSTALLER): $?"
done

rm -rf "$SOPHOS_INSTALL"

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