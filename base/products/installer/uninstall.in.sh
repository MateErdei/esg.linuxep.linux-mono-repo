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

function removeUserAndGroup()
{
  function check_user_exists()
  {
    grep  $1 /etc/passwd &>/dev/null
  }
  function check_group_exists()
  {
    grep  $1 /etc/group &>/dev/null
  }

  local USERNAME=$1
  local GROUPNAME=$2
  check_user_exists  $USERNAME  ||  echo "tried to remove user: $USERNAME, which does not exist"
  check_group_exists  $GROUPNAME  ||  echo "tried to remove group: $GROUPNAME, which does not exist"
  DELUSER=$(which deluser 2>/dev/null)
  USERDEL=$(which userdel 2>/dev/null)

  if [[ -x "$DELUSER" ]]
  then
      "$DELUSER" "$USERNAME" 2>/dev/null >/dev/null  || echo "Failed to delete user: $USERNAME"
  elif [[ -x "$USERDEL" ]]
  then
      "$USERDEL" "$USERNAME" 2>/dev/null >/dev/null  || echo "Failed to delete user: $USERNAME"
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
          check_group_exists  $GROUPNAME
          if [[ $? -eq 0 ]]
          then
              "$GROUP_DELETER" "$GROUPNAME" 2>/dev/null >/dev/null || echo "Failed to delete group: $GROUPNAME"
          fi
      else
          echo "Unable to delete group $GROUPNAME" >&2
      fi
  fi
}

if [[ -z $NO_REMOVE_USER ]]
then
  SOPHOS_SPL_USER_NAME="@SOPHOS_SPL_USER@"
  SOPHOS_SPL_GROUP_NAME="@SOPHOS_SPL_GROUP@"
  removeUserAndGroup ${SOPHOS_SPL_USER_NAME} ${SOPHOS_SPL_GROUP_NAME}

  NETWORK_USER_NAME="@SOPHOS_SPL_NETWORK@"
  NETWORK_GROUP_NAME="@SOPHOS_SPL_NETWORK@"
  removeUserAndGroup ${NETWORK_USER_NAME} ${NETWORK_GROUP_NAME}

  LOCAL_USER_NAME="@SOPHOS_SPL_LOCAL@"
  LOCAL_GROUP_NAME="@SOPHOS_SPL_LOCAL@"
  removeUserAndGroup ${LOCAL_USER_NAME} ${LOCAL_GROUP_NAME}
fi