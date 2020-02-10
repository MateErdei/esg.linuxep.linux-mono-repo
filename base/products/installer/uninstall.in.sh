#!/usr/bin/env bash

shopt -s nullglob

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
SOPHOS_INSTALL="${ABS_SCRIPTDIR%/bin*}"

if [ ! -f "${SOPHOS_INSTALL}/.sophos" ]
then
   echo "Uninstaller being run from unknown location."
   exit 1
fi

# Check the customer wants to uninstall
FORCE=0
while [[ $# -ge 1 ]]
do
    case $1 in
        --force)
            FORCE=1
            ;;
    esac
    shift
done

if (( $FORCE == 0 ))
then
    ChoiceMade=false
    while [[ ${ChoiceMade} == false ]]
    do
        read -p "Do you want to uninstall Sophos Linux Protection? " yn
        case $yn in
            [Yy] | [Yy][Ee][Ss] ) ChoiceMade=true;;
            [Nn] | [Nn][Oo] ) exit 1;;
            * ) echo "Please answer (y)es or (n)o.";;
        esac
    done
fi

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

# Uninstall plugins before stopping watchdog, so the plugins' uninstall scripts
# can stop the plugin process via wdctl.
PLUGIN_UNINSTALL_DIR="${SOPHOS_INSTALL}/base/update/var/installedproducts"
if [[ -d "$PLUGIN_UNINSTALL_DIR" ]]
then
    for UNINSTALLER in "$PLUGIN_UNINSTALL_DIR"/*
    do
        UNINSTALLER_BASE=${UNINSTALLER##*/}
        bash "$UNINSTALLER" || echo "Failed to uninstall $(UNINSTALLER_BASE): $?"
    done
else
    echo "Can't uninstall plugins: $PLUGIN_UNINSTALL_DIR doesn't exist"
fi

removeWatchdogSystemdService

rm -rf "$SOPHOS_INSTALL"

PATH=$PATH:/usr/sbin:/sbin

USERNAME="@SOPHOS_SPL_USER@"
GROUPNAME="@SOPHOS_SPL_GROUP@"
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