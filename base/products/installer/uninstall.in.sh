#!/usr/bin/env bash

shopt -s nullglob

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi
EXIT_CODE=0
FAILURE_UNKNOWN_LOCATION=1
FAILURE_REMOVE_UPDATE_SERVICE_FILES=2
FAILURE_REMOVE_WATCHDOG_SERVICE_FILES=3
FAILURE_REMOVE_PRODUCT_FILES=4
FAILURE_REMOVE_USER=5
FAILURE_REMOVE_GROUP=6

function failure()
{
  echo $1
  EXIT_CODE=$2
}
ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
SOPHOS_INSTALL="${ABS_SCRIPTDIR%/bin*}"

if [ ! -f "${SOPHOS_INSTALL}/.sophos" ]
then
   echo "Uninstaller being run from unknown location."
   exit ${FAILURE_UNKNOWN_LOCATION}
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


function unmountCommsComponentDependencies()
{
  CommsComponentChroot=$1
  for entry in etc/resolv.conf etc/hosts usr/lib usr/lib64 lib etc/ssl/certs etc/pki/tls/certs base/mcs/certs; do
    umount --force "${CommsComponentChroot}"/${entry}  > /dev/null 2>&1
  done
}

removeUpdaterSystemdService || failure "Failed to remove updating service files"  ${FAILURE_REMOVE_UPDATE_SERVICE_FILES}

# Uninstall plugins before stopping watchdog, so the plugins' uninstall scripts
# can stop the plugin process via wdctl.
PLUGIN_UNINSTALL_DIR="${SOPHOS_INSTALL}/base/update/var/installedproducts"
if [[ -d "$PLUGIN_UNINSTALL_DIR" ]]
then
    for UNINSTALLER in "$PLUGIN_UNINSTALL_DIR"/*
    do
        UNINSTALLER_BASE=${UNINSTALLER##*/}
        bash "$UNINSTALLER" || failure "Failed to uninstall $(UNINSTALLER_BASE): $?"
    done
else
    echo "Can't uninstall plugins: $PLUGIN_UNINSTALL_DIR doesn't exist"
fi

removeWatchdogSystemdService || failure "Failed to remove watchdog service files"  ${FAILURE_REMOVE_WATCHDOG_SERVICE_FILES}

CommsComponentChroot=${SOPHOS_INSTALL}/var/sophos-spl-comms
unmountCommsComponentDependencies "${CommsComponentChroot}"
rm -rf "$SOPHOS_INSTALL" || failure "Failed to remove all of $SOPHOS_INSTALL"  ${FAILURE_REMOVE_PRODUCT_FILES}

PATH=$PATH:/usr/sbin:/sbin

function removeUser()
{
  function check_user_exists()
  {
    grep  "$1" /etc/passwd &>/dev/null
  }

  local USERNAME=$1
  USER_DELETER=$(which deluser 2>/dev/null)
  [[ -x "$USER_DELETER" ]] || USER_DELETER=$(which userdel 2>/dev/null)

  if [[ -x "$USER_DELETER" ]]
  then
      check_user_exists "$USERNAME"
      USER_EXISTS=$?
      local USER_DELETE_TRIES=0
      while [ $USER_EXISTS -eq 0 ] && [ $USER_DELETE_TRIES -le 5 ]
      do
          sleep 1
          "$USER_DELETER" "$USERNAME" 2>/dev/null >/dev/null && USER_EXISTS=1
          USER_DELETE_TRIES=$((USER_DELETE_TRIES+1))
      done
      check_user_exists "$USERNAME"
      [ $? -eq 0 ] && failure "Failed to delete user: $USERNAME"  ${FAILURE_REMOVE_USER}
  else
      echo "Unable to delete user $USERNAME" >&2
  fi
}

function removeGroup()
{
  function check_group_exists()
  {
    grep  "$1" /etc/group &>/dev/null
  }

  local GROUPNAME=$1
  GROUP_DELETER=$(which delgroup 2>/dev/null)
  [[ -x "$GROUP_DELETER" ]] || GROUP_DELETER=$(which groupdel 2>/dev/null)
  if [[ -x "$GROUP_DELETER" ]]
  then
      check_group_exists  "$GROUPNAME"
      GROUP_EXISTS=$?
      GROUP_DELETE_TRIES=0
      while [ $GROUP_EXISTS -eq 0 ] && [ $GROUP_DELETE_TRIES -le 5 ]
      do
          sleep 1
          "$GROUP_DELETER" "$GROUPNAME" 2>/dev/null >/dev/null && GROUP_EXISTS=1
          GROUP_DELETE_TRIES=$((GROUP_DELETE_TRIES+1))
      done
      check_group_exists "$GROUPNAME"
      [ $? -eq 0 ] && failure "Failed to delete group: $GROUPNAME"  ${FAILURE_REMOVE_GROUP}
  else
      echo "Unable to delete group $GROUPNAME" >&2
  fi
}

if [[ -z $NO_REMOVE_USER ]]
then
  SOPHOS_SPL_USER_NAME="@SOPHOS_SPL_USER@"
  removeUser    ${SOPHOS_SPL_USER_NAME}

  NETWORK_USER_NAME="@SOPHOS_SPL_NETWORK@"
  removeUser    ${NETWORK_USER_NAME}

  LOCAL_USER_NAME="@SOPHOS_SPL_LOCAL@"
  removeUser    ${LOCAL_USER_NAME}

  SOPHOS_SPL_GROUP_NAME="@SOPHOS_SPL_GROUP@"
  removeGroup   ${SOPHOS_SPL_GROUP_NAME}
fi

exit $EXIT_CODE