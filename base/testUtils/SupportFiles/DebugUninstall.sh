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
FAILURE_TO_BACKUP_FILES=7
FAILURE_REMOVE_DIAGNOSE_SERVICE_FILES=8
function failure()
{
  echo $1
  EXIT_CODE=$2
}
ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)
SOPHOS_INSTALL=${SOPHOS_INSTALL:-"/opt/sophos-spl"}

if [ ! -f "${SOPHOS_INSTALL}/.sophos" ]
then
   echo "Uninstaller being run from unknown location."
   exit ${FAILURE_UNKNOWN_LOCATION}
fi

# Check the customer wants to uninstall
FORCE=0
DOWNGRADE=0

while [[ $# -ge 1 ]]
do
    case $1 in
        --force)
            FORCE=1
            ;;
        --downgrade)
            DOWNGRADE=1
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

function removeDiagnoseSystemdService()
{
    systemctl stop sophos-spl-diagnose.service
    rm -f "/lib/systemd/system/sophos-spl-diagnose.service"
    rm -f "/usr/lib/systemd/system/sophos-spl-diagnose.service"
    systemctl daemon-reload
}

function removeWatchdogSystemdService()
{
    systemctl stop sophos-spl.service
    rm -f "/lib/systemd/system/sophos-spl.service"
    rm -f "/usr/lib/systemd/system/sophos-spl.service"
    rm -f "/etc/systemd/system/multi-user.target.wants/sophos-spl.service"
    systemctl daemon-reload
}

if (( $DOWNGRADE == 0 ))
then
  removeUpdaterSystemdService || failure "Failed to remove updating service files"  ${FAILURE_REMOVE_UPDATE_SERVICE_FILES}
fi

# Uninstall plugins before stopping watchdog, so the plugins' uninstall scripts
# can stop the plugin process via wdctl.
PLUGIN_UNINSTALL_DIR="${SOPHOS_INSTALL}/base/update/var/installedproducts"
if [[ -d "$PLUGIN_UNINSTALL_DIR" ]]
then
    for UNINSTALLER in "$PLUGIN_UNINSTALL_DIR"/*
    do
        UNINSTALLER_BASE=${UNINSTALLER##*/}
        if (( $DOWNGRADE == 0 ))
        then
          bash "$UNINSTALLER" || failure "Failed to uninstall ${UNINSTALLER_BASE}: $?"
        else
          bash $UNINSTALLER --downgrade || failure "Failed to uninstall ${UNINSTALLER_BASE}: $?"
        fi
    done
else
    echo "Can't uninstall plugins: $PLUGIN_UNINSTALL_DIR doesn't exist"
fi

removeWatchdogSystemdService || failure "Failed to remove watchdog service files"  ${FAILURE_REMOVE_WATCHDOG_SERVICE_FILES}
removeDiagnoseSystemdService || failure "Failed to remove diagnose service files"  ${FAILURE_REMOVE_DIAGNOSE_SERVICE_FILES}


if (( $DOWNGRADE == 0 ))
then
  rm -rf "$SOPHOS_INSTALL" || failure "Failed to remove all of $SOPHOS_INSTALL"  ${FAILURE_REMOVE_PRODUCT_FILES}
else
  input=$SOPHOS_INSTALL/base/etc/backupfileslist.dat
  while IFS= read -r line
  do
    if [[ -f "$SOPHOS_INSTALL/$line" ]]
    then
      DIR=${line%/*}
      mkdir -p "$SOPHOS_INSTALL/$DIR/downgrade-backup"
      mv "$SOPHOS_INSTALL/$line" "$SOPHOS_INSTALL/$DIR/downgrade-backup/" || failure "Failed to copy $line"  ${FAILURE_TO_BACKUP_FILES}
    elif [[ -d "$SOPHOS_INSTALL/$line" ]]
    then
      mkdir -p "$SOPHOS_INSTALL/tmp/downgrade-backup"
      cp -r "$SOPHOS_INSTALL/$line/"* "$SOPHOS_INSTALL/tmp/downgrade-backup" || failure "Failed to copy $line"  ${FAILURE_TO_BACKUP_FILES}
      rm -rf "${SOPHOS_INSTALL}/${line}/"*  || failure "Failed to remove $line"  ${FAILURE_TO_BACKUP_FILES}
      mv "$SOPHOS_INSTALL/tmp/downgrade-backup" "$SOPHOS_INSTALL/$line/downgrade-backup" || failure "Failed to move $line"  ${FAILURE_TO_BACKUP_FILES}
    fi
  done < "$input"
  input=$SOPHOS_INSTALL/base/etc/downgradepaths.dat
  while IFS= read -r line
  do
    rm -rf "$SOPHOS_INSTALL/$line" || failure "Failed to remove file/folder $line"  ${FAILURE_REMOVE_PRODUCT_FILES}
  done < "$input"
  chown sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
  chown -R sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/policy
  chown -R sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/action
  chown -R sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/event
  chown -R sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/status
  chown -R sophos-spl-user:sophos-spl-group  ${SOPHOS_INSTALL}/base/mcs/response
  chown root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/mcs.config
  chown root:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/machine_id.txt
fi

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
      while [ $USER_EXISTS -eq 0 ] && [ $USER_DELETE_TRIES -le 10 ]
      do
          "$USER_DELETER" "$USERNAME" 2>/dev/null >/dev/null && USER_EXISTS=1
          if [[ $USER_EXISTS == 0 ]]
          then
              sleep 1
              ps -u "$USERNAME"
              USER_DELETE_TRIES=$((USER_DELETE_TRIES+1))
          fi
      done
      check_user_exists "$USERNAME"
      if [[ $? -eq 0 ]]
      then
          echo "Running processes as $USERNAME:"
          ps -u "$USERNAME"
          echo "userdel output:"
          "$USER_DELETER" "$USERNAME"
          echo "userdel -f output:"
          "$USER_DELETER" -f "$USERNAME"
          check_user_exists "$USERNAME"
          ## Always fail so that we can debug what's gone wrong
          failure "Failed to delete user: $USERNAME: delete -f=$?"  ${FAILURE_REMOVE_USER}
      fi
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
      while [ $GROUP_EXISTS -eq 0 ] && [ $GROUP_DELETE_TRIES -le 10 ]
      do
          "$GROUP_DELETER" "$GROUPNAME" 2>/dev/null >/dev/null && GROUP_EXISTS=1
          if [[ $GROUP_EXISTS -eq 0 ]]
          then
              sleep 1
              GROUP_DELETE_TRIES=$((GROUP_DELETE_TRIES+1))
              ps -g "${GROUPNAME}"
          fi
      done
      check_group_exists "$GROUPNAME"
      [ $? -eq 0 ] && failure "Failed to delete group: $GROUPNAME"  ${FAILURE_REMOVE_GROUP}
  else
      echo "Unable to delete group $GROUPNAME" >&2
  fi
}

if [[ -z $NO_REMOVE_USER ]]
then
  if [[ ${DOWNGRADE} == 0 ]]
  then
    SOPHOS_SPL_USER_NAME="sophos-spl-user"
    removeUser    ${SOPHOS_SPL_USER_NAME}

    LOCAL_USER_NAME="sophos-spl-local"
    removeUser    ${LOCAL_USER_NAME}

    UPDATESCHEDULER_USER_NAME="sophos-spl-updatescheduler"
    removeUser    ${UPDATESCHEDULER_USER_NAME}

    SOPHOS_SPL_GROUP_NAME="sophos-spl-group"
    removeGroup   ${SOPHOS_SPL_GROUP_NAME}

    SOPHOS_SPL_IPC_GROUP="sophos-spl-ipc"
    removeGroup   ${SOPHOS_SPL_IPC_GROUP}
  fi
fi

exit $EXIT_CODE
