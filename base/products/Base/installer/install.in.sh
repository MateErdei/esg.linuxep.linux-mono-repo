#!/usr/bin/env bash
set -x

EXIT_FAIL_CREATE_DIRECTORY=10
EXIT_FAIL_FIND_GROUPADD=11
EXIT_FAIL_ADD_GROUP=12
EXIT_FAIL_FIND_USERADD=13
EXIT_FAIL_ADDUSER=14
EXIT_FAIL_FIND_GETENT=15
EXIT_FAIL_WDCTL_FAILED_TO_COPY=16
EXIT_FAIL_NOT_ROOT=17
EXIT_FAIL_DIR_MARKER=18
EXIT_FAIL_FIND_USERMOD=19
EXIT_FAIL_VERSIONEDCOPY=20
EXIT_FAIL_REGISTER=30
EXIT_FAIL_SERVICE=40
EXIT_FAIL_WRONG_LIBC_VERSION=50

umask 077

PRODUCT_LINE_ID="ServerProtectionLinux-Base-component"
BUILD_LIBC_VERSION=@BUILD_SYSTEM_LIBC_VERSION@
system_libc_version=$(ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev)

STARTINGDIR=$(pwd)
SCRIPTDIR=${0%/*}
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR=${STARTINGDIR}
fi

ABS_SCRIPTDIR=$(cd "${SCRIPTDIR}" && pwd)

source "${ABS_SCRIPTDIR}/cleanupinstall.sh"
source "${ABS_SCRIPTDIR}/checkAndRunExtraUpgrade.sh"

[[ -n "$DIST" ]] || DIST=$ABS_SCRIPTDIR

MCS_TOKEN=${MCS_TOKEN:-}
MCS_URL=${MCS_URL:-}
MCS_MESSAGE_RELAYS=${MCS_MESSAGE_RELAYS:-}
MCS_CA=${MCS_CA:-}
PRODUCT_ARGUMENTS=${PRODUCT_ARGUMENTS:-}
CUSTOMER_TOKEN=${CUSTOMER_TOKEN:-}
SOPHOS_LOG_LEVEL=${SOPHOS_LOG_LEVEL:-}

function failure()
{
    local CODE=$1
    shift
    echo "$@" >&2
    exit $CODE
}

while [[ $# -ge 1 ]] ; do
    case $1 in
        --instdir | --install)
            shift
            export SOPHOS_INSTALL=$1
            ;;
        --mcs-token)
            shift
            MCS_TOKEN=$1
            ;;
        --mcs-url)
            shift
            MCS_URL=$1
            ;;
        --mcs-ca)
            shift
            MCS_CA=$1
            ;;
        --allow-override-mcs-ca)
            ALLOW_OVERRIDE_MCS_CA=--allow-override-mcs-ca
            ;;
        --mcs-config)
            shift
            PREREGISTED_MCS_CONFIG=$1
            [[ -f ${PREREGISTED_MCS_CONFIG} ]] || failure 2 "MCS config \"${PREREGISTED_MCS_CONFIG}\" does not exist"
            ;;
        --mcs-policy-config)
            shift
            PREREGISTED_MCS_POLICY_CONFIG=$1
            [[ -f ${PREREGISTED_MCS_POLICY_CONFIG} ]] || failure 2 "MCS policy config \"${PREREGISTED_MCS_POLICY_CONFIG}\" does not exist"
            ;;
        --log-level|--sophos-log-level|--loglevel)
            shift
            SOPHOS_LOG_LEVEL=$1
            ;;
        *)
            failure 2 "BAD OPTION $1"
            ;;
    esac
    shift
done

function isServiceInstalled()
{
    local TARGET="$1"
    systemctl list-unit-files | grep -q "^${TARGET}\b" >/dev/null
}

CORESETTING=""
DEBUGENV=""
if [[ -n $SOPHOS_CORE_DUMP_ON_PLUGIN_KILL ]]
then
  CORESETTING="LimitCORE=infinity"
  DEBUGENV="Environment='SOPHOS_CORE_DUMP_ON_PLUGIN_KILL=1'"
elif [[ -n $SOPHOS_ENABLE_CORE_DUMP ]]
then
  CORESETTING="LimitCORE=infinity"
fi

function createWatchdogSystemdService()
{
    NEW_SERVICE_INFO=$(cat <<EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}"
${DEBUGENV}
ExecStart="${SOPHOS_INSTALL}/base/bin/sophos_watchdog"
Restart=always
KillMode=mixed
Delegate=yes
${CORESETTING}

[Install]
WantedBy=multi-user.target

[Unit]
Description=Sophos Linux Protection
RequiresMountsFor="${SOPHOS_INSTALL}"
EOF
)
    if [[ -d "/lib/systemd/system" ]]
        then
            STARTUP_DIR="/lib/systemd/system"
        elif [[ -d "/usr/lib/systemd/system" ]]
        then
            STARTUP_DIR="/usr/lib/systemd/system"
        else
            failure ${EXIT_FAIL_SERVICE} "Could not find systemd service directory"
    fi

    EXISTING_SERVICE_INFO=""
    if [[ -f "${STARTUP_DIR}/sophos-spl.service" ]]
    then
        EXISTING_SERVICE_INFO=$(<${STARTUP_DIR}/sophos-spl.service)
    fi

    if [[ "${EXISTING_SERVICE_INFO}" != "${NEW_SERVICE_INFO}" || $FORCE_INSTALL ]]
    then
        cat > "${STARTUP_DIR}/sophos-spl.service" <<EOF
${NEW_SERVICE_INFO}
EOF
        chmod 644 "${STARTUP_DIR}/sophos-spl.service"
        systemctl daemon-reload
        systemctl enable --quiet sophos-spl.service
    fi
}

function stopSsplService()
{
    systemctl stop sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to stop sophos-spl service"
}

function startSsplService()
{
    systemctl start sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to restart sophos-spl service"
}

function createUpdaterSystemdService()
{
    if ! isServiceInstalled sophos-spl-update.service || $FORCE_INSTALL
    then
        if [[ -d "/lib/systemd/system" ]]
        then
            STARTUP_DIR="/lib/systemd/system"
        elif [[ -d "/usr/lib/systemd/system" ]]
        then
            STARTUP_DIR="/usr/lib/systemd/system"
        else
            failure ${EXIT_FAIL_SERVICE} "Could not install the sophos-spl update service"
        fi
        local service_name="sophos-spl-update.service"

        cat > "${STARTUP_DIR}/${service_name}" << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}"
ExecStart="${SOPHOS_INSTALL}/base/bin/SulDownloader" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report.json"
Restart=no

[Unit]
Description=Sophos Server Protection Update Service
RequiresMountsFor="${SOPHOS_INSTALL}"
EOF
        chmod 644 "${STARTUP_DIR}/${service_name}"
        systemctl daemon-reload
    fi
}

function createDiagnoseSystemdService()
{
    if ! isServiceInstalled sophos-spl-diagnose.service || $FORCE_INSTALL
    then
        if [[ -d "/lib/systemd/system" ]]
        then
            STARTUP_DIR="/lib/systemd/system"
        elif [[ -d "/usr/lib/systemd/system" ]]
        then
            STARTUP_DIR="/usr/lib/systemd/system"
        else
            failure ${EXIT_FAIL_SERVICE} "Could not install the sophos-spl diagnose service"
        fi
        local service_name="sophos-spl-diagnose.service"

        cat > "${STARTUP_DIR}/${service_name}" << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}"

ExecStart="${SOPHOS_INSTALL}/bin/sophos_diagnose" --remote
Restart=no

[Unit]
Description=Sophos Server Protection Diagnose
RequiresMountsFor="${SOPHOS_INSTALL}"
EOF
        chmod 644 "${STARTUP_DIR}/${service_name}"
        systemctl daemon-reload
    fi
}
function confirmProcessRunning()
{
    pgrep -f "$1" &> /dev/null
}

function waitForProcess()
{
    for (( deadline = $SECONDS + 30; $SECONDS < $deadline; ))
    do
        confirmProcessRunning "$@" && return 0
        sleep 1
    done

    return 1
}

function makedir()
{
    # Creates directory and enforces it's permissions
    if [[ ! -d "$2" ]]
    then
        mkdir -p "$2" || failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to create directory: $2"
    fi
    chmod "$1" "$2" || failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to apply correct permissions: $1 to directory: $2"
}

function makeRootDirectory()
{
    local install_path="${SOPHOS_INSTALL%/*}"

    # Make sure that the install_path string is not empty, in the case of "/foo"
    if [[ -z "${install_path}" ]]
    then
        install_path="/"
    fi

    while [[ -n "${install_path}" ]] && [[ ! -d "${install_path}" ]]
    do
        install_path="${install_path%/*}"
    done

    local createDirs="${SOPHOS_INSTALL#$install_path/}"

    #Following loop requires trailing slash
    if [[ ${createDirs:-1} != "/" ]]
    then
        local createDirs="$createDirs/"
    fi

    #Iterate through directories giving minimum execute permissions to allow sophos-spl user to run executables
    while [[ -n "$createDirs" ]]
    do
        currentDir="${createDirs%%/*}"
        install_path="${install_path}/${currentDir}"
        makedir 711 "${install_path}"
        createDirs="${createDirs#$currentDir/}"
    done
}


if [[ $(id -u) != 0 ]]
then
    failure ${EXIT_FAIL_NOT_ROOT} "Please run this installer as root."
fi

function build_version_less_than_system_version()
{
    test "$(printf '%s\n' ${BUILD_LIBC_VERSION} "${system_libc_version}" | sort -V | head -n 1)" != ${BUILD_LIBC_VERSION}
}

function add_group()
{
  local groupname="$1"

  GROUPADD="$(which groupadd)"
  [[ -x "${GROUPADD}" ]] || GROUPADD=/usr/sbin/groupadd
  [[ -x "${GROUPADD}" ]] || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to find groupadd to add low-privilege group"

  if [[ -f "$INSTALL_OPTIONS_FILE" ]]
  then
    while read -r line; do
      if [[ "$line" =~ ^--group-ids-to-configure=?(.*)$ ]]
        then
          IFS=',' read -ra GIDS_ARRAY <<< "${line/*=/}"
          for gid in "${GIDS_ARRAY[@]}";
          do
            if [[ "${gid%%:*}" == "$groupname" ]]
            then
              groupID="-g ${gid#*:}"
              break
            fi
          done
      fi
    done <"$INSTALL_OPTIONS_FILE"
  fi

  addGroupCmd="${GROUPADD} -r ${groupname}"
  if [[ -z "$("${GETENT}" group "${groupname}")" ]]
  then
    if [[ -z "$groupID" ]]
      then
         ${addGroupCmd} || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to add group ${groupname}"
      else
        ${addGroupCmd} "${groupID}" || ${addGroupCmd} || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to add group ${groupname}"
      fi
  fi
}

function add_user()
{
  local username="$1"
  local groupname="$2"

  USERADD="$(which useradd)"
  [[ -x "${USERADD}" ]] || USERADD=/usr/sbin/useradd
  [[ -x "${USERADD}" ]] || failure ${EXIT_FAIL_ADDUSER} "Failed to find useradd to add low-privilege user"

  if [[ -f "$INSTALL_OPTIONS_FILE" ]]
  then
    while read -r line; do
      if [[ "$line" =~ ^--user-ids-to-configure=?(.*)$ ]]
        then
          IFS=',' read -ra UIDS_ARRAY <<< "${line/*=/}"
          for uid in "${UIDS_ARRAY[@]}";
          do
            if [[ "${uid%%:*}" == "$username" ]]
            then
              userId="-u ${uid#*:}"
              break
            fi
          done
      fi
    done <"$INSTALL_OPTIONS_FILE"
  fi

  addUserCmd="${USERADD} -g ${groupname} -M -N -r -s /bin/false ${username}"
  if [[ -z "$("${GETENT}" passwd "${username}")" ]]
    then
      if [[ -z "${userId}" ]]
      then
        ${addUserCmd} -d "${SOPHOS_INSTALL}" || failure ${EXIT_FAIL_ADDUSER} "Failed to add user ${username}"
      else
        ${addUserCmd} "${userId}"  -d "${SOPHOS_INSTALL}" \
          || ${addUserCmd} -d "${SOPHOS_INSTALL}"  \
          || failure ${EXIT_FAIL_ADDUSER} "Failed to add user ${username}"
      fi
  fi
}

function add_to_group()
{
    local username="$1"
    local groupname="$2"
    USERMOD="$(which usermod)"
    [[ -x "${USERMOD}" ]] || USERMOD=/usr/sbin/usermod
    [[ -x "${USERMOD}" ]] || failure ${EXIT_FAIL_FIND_USERMOD} "Failed to find usermod to add new user to group"
    "${USERMOD}" -a -G "$groupname" "$username" || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $username to group $groupname"
}

function generate_local_user_group_id_config()
{
  [[ -d "${SOPHOS_INSTALL}/base/etc" ]] || failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to generate user and group ID template file, ${SOPHOS_INSTALL}/base/etc dir does not exist"
  local configPath="${SOPHOS_INSTALL}/base/etc/user-group-ids-requested.conf"
  if [[ ! -f "${configPath}" ]]
  then
    if [[ -f "${BASE_INSTALL_OPTIONS_FILE}" ]]
    then
      jsonString="{"
      while read -r line; do
        if [[ "$line" =~ ^--user-ids-to-configure=?(.*)$ ]]
          then
            jsonString="${jsonString}\"users\":{"
            IFS=',' read -ra UIDS_ARRAY <<< "${line/*=/}"
            for uid in "${UIDS_ARRAY[@]}"; do
              jsonString="${jsonString}"\"${uid%%:*}\"":${uid#*:},"
            done
            jsonString="${jsonString}},"
        fi
        if [[ "$line" =~ ^--group-ids-to-configure=?(.*)$ ]]
          then
            jsonString="${jsonString}\"groups\":{"
            IFS=',' read -ra GIDS_ARRAY <<< "${line/*=/}"
            for gid in "${GIDS_ARRAY[@]}"; do
              jsonString="${jsonString}"\"${gid%%:*}\"":${gid#*:},"
            done
            jsonString="${jsonString}},"
        fi
      done <"${BASE_INSTALL_OPTIONS_FILE}"
      jsonString=$(sed 's/,}/}/g' <<< "${jsonString}}")
    fi

    cat > "${configPath}" << EOF
# This file allows the user and group IDs of the SPL product to be specified
# note - This file is used to request user and group ID changes whereas user-group-ids-actual.conf reflects the IDs currently in use by the product.
# Before modifying this file, please stop the product: 'systemctl stop sophos-spl'
# To maintain the security of the endpoint, the following considerations should be made when requesting ID changes
#  - User or group name should not be 'root'
#  - User or group ID should not be 0
#  - User or group IDs should be unique to both existing users and groups on the machine and within this config file
# Requested user and group IDs can be made using the following template:
#{"users":{"username1":uid1,"username2":uid2},"groups":{"groupname1":gid1,"groupname2":gid2}}
${jsonString}
EOF
  fi
  chown root:root "${configPath}"
  chmod 600 "${configPath}"
}

function cleanup_comms_component()
{
  function check_user_exists()
  {
    getent passwd "$1"
  }
  function check_group_exists()
  {
    getent group "$1"
  }

  local COMMSUSER="sophos-spl-network"

  USER_DELETER=$(which deluser 2>/dev/null)
  [[ -x "$USER_DELETER" ]] || USER_DELETER=$(which userdel 2>/dev/null)
  if [[ -x "$USER_DELETER" ]]
  then
      check_user_exists "$COMMSUSER"
      local USER_EXISTS=$?
      local USER_DELETE_TRIES=0
      while (( $USER_EXISTS == 0 && $USER_DELETE_TRIES <= 10 ))
      do
          (( $USER_DELETE_TRIES > 0 )) && sleep 1
          "$USER_DELETER" "$COMMSUSER" 2>/dev/null >/dev/null && USER_EXISTS=1
          USER_DELETE_TRIES=$((USER_DELETE_TRIES+1))
      done
      check_user_exists "$COMMSUSER" && echo "Warning: Failed to delete user: $COMMSUSER"
  else
      echo "Unable to delete user $COMMSUSER" >&2
  fi

  GROUP_DELETER=$(which delgroup 2>/dev/null)
  [[ -x "$GROUP_DELETER" ]] || GROUP_DELETER=$(which groupdel 2>/dev/null)
  if [[ -x "$GROUP_DELETER" ]]
  then
      check_group_exists "$COMMSUSER"
      local GROUP_EXISTS=$?
      local GROUP_DELETE_TRIES=0
      while (( $GROUP_EXISTS == 0 && $GROUP_DELETE_TRIES <= 10 ))
      do
          (( $GROUP_DELETE_TRIES > 0 )) && sleep 1
          "$GROUP_DELETER" "$COMMSUSER" 2>/dev/null >/dev/null && GROUP_EXISTS=1
          GROUP_DELETE_TRIES=$((GROUP_DELETE_TRIES+1))
      done
      check_group_exists "$COMMSUSER" && echo "Warning: Failed to delete group: $COMMSUSER"
  else
      echo "Unable to delete group $COMMSUSER" >&2
  fi

  [[ -d "${SOPHOS_INSTALL}/logs/base/sophos-spl-comms" ]] && unlink "${SOPHOS_INSTALL}/logs/base/sophos-spl-comms"

  if [[ -d "${SOPHOS_INSTALL}/var/sophos-spl-comms" ]]
  then
    for entry in lib usr/lib etc/hosts etc/resolv.conf usr/lib64 etc/ssl/certs etc/pki/tls/certs etc/pki/ca-trust/extracted base/mcs/certs base/remote-diagnose/output;
    do
        local mountDir="${SOPHOS_INSTALL}/var/sophos-spl-comms/${entry}"
        local timeout=$(($(date +%s) + 60))
        if [[ -e "${mountDir}" ]]
        then
          # Will retry until mount is removed or 60 second timeout is exceeded
          until umount --force "${mountDir}" > /dev/null 2>&1 || [[ $(date +%s) -gt "${timeout}" ]]; do :
          done
        fi
    done
    rm -rf "${SOPHOS_INSTALL}/var/sophos-spl-comms" > /dev/null 2>&1
  fi

  [[ -d "${SOPHOS_INSTALL}/var/comms" ]] && rm -rf "${SOPHOS_INSTALL}/var/comms"
  [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log" ]] && rm "${SOPHOS_INSTALL}/logs/base/sophosspl/comms_component.log"*
  [[ -f "${SOPHOS_INSTALL}/base/pluginRegistry/commscomponent.json" ]] && rm "${SOPHOS_INSTALL}/base/pluginRegistry/commscomponent.json"
}

if build_version_less_than_system_version
then
    failure ${EXIT_FAIL_WRONG_LIBC_VERSION} "Failed to install on unsupported system. Detected GLIBC version ${system_libc_version} < required ${BUILD_LIBC_VERSION}"
fi


export DIST
export SOPHOS_INSTALL

## Add a low-privilege group
GROUP_NAME=@SOPHOS_SPL_GROUP@
SOPHOS_SPL_IPC_GROUP=@SOPHOS_SPL_IPC_GROUP@

GETENT=/usr/bin/getent
[[ -x "${GETENT}" ]] || GETENT=$(which getent)
[[ -x "${GETENT}" ]] || failure ${EXIT_FAIL_FIND_GETENT} "Failed to find getent"

add_group "${GROUP_NAME}"
add_group "${SOPHOS_SPL_IPC_GROUP}"

makeRootDirectory "${SOPHOS_INSTALL}"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}"

# Adds a hidden file to mark the install directory which is used by the uninstaller.
touch "${SOPHOS_INSTALL}/.sophos" || failure ${EXIT_FAIL_DIR_MARKER} "Failed to create install directory marker file"

## Add a low-privilege users
USER_NAME=@SOPHOS_SPL_USER@
LOCAL_USER_NAME=@SOPHOS_SPL_LOCAL@
UPDATESCHEDULER_USER_NAME=@SOPHOS_SPL_UPDATESCHEDULER@

add_user "${USER_NAME}" "${GROUP_NAME}"
add_user "${LOCAL_USER_NAME}" "${GROUP_NAME}"
add_user "${UPDATESCHEDULER_USER_NAME}" "${GROUP_NAME}"

# Add users to the IPC group which need to read and write the IPC socket
add_to_group "${USER_NAME}" "${SOPHOS_SPL_IPC_GROUP}"
add_to_group "${UPDATESCHEDULER_USER_NAME}" "${SOPHOS_SPL_IPC_GROUP}"

makedir 1770 "${SOPHOS_INSTALL}/tmp"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp"

check_for_upgrade "${SOPHOS_INSTALL}/base/VERSION.ini" ${PRODUCT_LINE_ID} "${DIST}"

# Var directories
makedir 711 "${SOPHOS_INSTALL}/var"
makedir 710 "${SOPHOS_INSTALL}/var/ipc"
makedir 770 "${SOPHOS_INSTALL}/var/ipc/plugins"
chown "${USER_NAME}:${SOPHOS_SPL_IPC_GROUP}" "${SOPHOS_INSTALL}/var/ipc"
chown "${USER_NAME}:${SOPHOS_SPL_IPC_GROUP}" "${SOPHOS_INSTALL}/var/ipc/plugins"

makedir 770 "${SOPHOS_INSTALL}/var/lock-sophosspl"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/lock-sophosspl"
if [[ -f "${SOPHOS_INSTALL}/var/lock-sophosspl/mcsrouter.pid" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/lock-sophosspl/mcsrouter.pid"*
fi
# Update scheduler needs to be able to access this lock to check if suldownloader is running.
if [[ -f "${SOPHOS_INSTALL}/var/lock-sophosspl/suldownloader.pid" ]]
then
    chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/lock-sophosspl/suldownloader.pid"
    chmod 0660 "${SOPHOS_INSTALL}/var/lock-sophosspl/suldownloader.pid"
fi

makedir 700 "${SOPHOS_INSTALL}/var/lock"

makedir 711 "${SOPHOS_INSTALL}/var/cache"
makedir 700 "${SOPHOS_INSTALL}/var/cache/mcs_fragmented_policies"
chown "${LOCAL_USER_NAME}:root" "${SOPHOS_INSTALL}/var/cache/mcs_fragmented_policies"

makedir 770 "${SOPHOS_INSTALL}/var/sophosspl"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/sophosspl"

# Log directories
makedir 711 "${SOPHOS_INSTALL}/logs"
makedir 711 "${SOPHOS_INSTALL}/logs/base"
makedir 711 "${SOPHOS_INSTALL}/logs/installation"
makedir 770 "${SOPHOS_INSTALL}/logs/base/sophosspl"
chown "root:" "${SOPHOS_INSTALL}/logs/base"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl"

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log" ]]
then
    chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/remote_diagnose.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log" ]]
then
    chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/sophos_managementagent.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log" ]]
then
    chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/telemetry.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/tscheduler.log" ]]
then
    chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/tscheduler.log"*
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/instance-metadata.json" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/instance-metadata.json"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/instance-metadata.json"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/mcs.config" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/mcs.config"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/mcs.config"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/current_proxy"
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/updatescheduler.log"
fi

if [[ -f "${SOPHOS_INSTALL}/base/telemetry/cache/updatescheduler-telemetry.json" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/telemetry/cache/updatescheduler-telemetry.json"
fi

if [ -f "${SOPHOS_INSTALL}/var/sophosspl/upgrade_marker_file" ]
then
  chown -h "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/sophosspl/upgrade_marker_file"
  chmod 640 "${SOPHOS_INSTALL}/var/sophosspl/upgrade_marker_file"
fi

if [ -f "${SOPHOS_INSTALL}/var/sophosspl/outbreak_status.json" ]
then
  chown -h "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/sophosspl/outbreak_status.json"
  chmod 640 "${SOPHOS_INSTALL}/var/sophosspl/outbreak_status.json"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json"
    chmod 600 "${SOPHOS_INSTALL}/base/etc/sophosspl/flags-mcs.json"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/sophosspl/datafeed_tracker" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/datafeed_tracker"
    chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/datafeed_tracker"
fi

if [[ -f "${SOPHOS_INSTALL}/tmp/.upgradeToNewWarehouse" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp/.upgradeToNewWarehouse"
    chmod 660 "${SOPHOS_INSTALL}/tmp/.upgradeToNewWarehouse"
fi

if [[ -f "${SOPHOS_INSTALL}/base/etc/install_options" ]]
then
    chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/install_options"
    chmod 440 "${SOPHOS_INSTALL}/base/etc/install_options"
fi

makedir 711 "${SOPHOS_INSTALL}/base"
makedir 711 "${SOPHOS_INSTALL}/base/etc"
makedir 750 "${SOPHOS_INSTALL}/base/etc/sophosspl"
chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl"

makedir 750 "${SOPHOS_INSTALL}/base/pluginRegistry"
chown -R "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/pluginRegistry"

makedir 710 "${SOPHOS_INSTALL}/base/update"
makedir 700 "${SOPHOS_INSTALL}/base/update/cache/"
makedir 700 "${SOPHOS_INSTALL}/base/update/rootcerts"
makedir 710 "${SOPHOS_INSTALL}/base/update/var"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/installedproducts"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/installedproductversions"
# Reset permissions on base/update so that they are locked down.
chown -R "root:root" "${SOPHOS_INSTALL}/base/update"

# Allow group to access (but not read or write) these so that the updatescheduler user can reach it's own directories
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update/var"
makedir 700 "${SOPHOS_INSTALL}/base/update/updatecachecerts"
chown -R "${UPDATESCHEDULER_USER_NAME}:root" "${SOPHOS_INSTALL}/base/update/updatecachecerts"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/updatescheduler"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports"
# Move old update reports into new location for upgrades coming from pre-xdr

if [[ -d "${SOPHOS_INSTALL}/base/update/var/processedReports" ]]
then
  mv "${SOPHOS_INSTALL}"/base/update/var/processedReports/update_report*.json "${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports/" 2>&1 > /dev/null
  mv "${SOPHOS_INSTALL}"/base/update/var/update_report*.json "${SOPHOS_INSTALL}/base/update/var/updatescheduler/" 2>&1 > /dev/null
  rm -rf "${SOPHOS_INSTALL}/base/update/var/processedReports"
fi
chown -R "${UPDATESCHEDULER_USER_NAME}:root" "${SOPHOS_INSTALL}/base/update/var/updatescheduler"

if [[ -f "${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json"
    chmod 600 "${SOPHOS_INSTALL}/base/update/var/updatescheduler/previous_update_config.json"
fi

if [[ -f "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json"
    chmod 600 "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_config.json"
fi

if [[ -f "${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json"
    chmod 600 "${SOPHOS_INSTALL}/base/update/var/updatescheduler/state_machine_raw_data.json"
fi

if [[ -n "$(ls -A "${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports/" 2>/dev/null)" ]]
then
    chown "${UPDATESCHEDULER_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports/"update*.json
    chmod 600 "${SOPHOS_INSTALL}/base/update/var/updatescheduler/processedReports/"update*.json
fi

makedir 711 "${SOPHOS_INSTALL}/base/bin"
makedir 711 "${SOPHOS_INSTALL}/base/lib64"

if [[ -d "${SOPHOS_INSTALL}/base/mcs/status" ]]
then
    chmod -R 660 "${SOPHOS_INSTALL}/base/mcs/status"
fi

if [[ -d "${SOPHOS_INSTALL}/base/mcs/internal_policy" ]]
then
    chmod -R 660 "${SOPHOS_INSTALL}/base/mcs/internal_policy"
fi

if [[ -d "${SOPHOS_INSTALL}/base/mcs/policy" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/policy"
    chmod -R 640 "${SOPHOS_INSTALL}/base/mcs/policy"
fi

if [[ -d "${SOPHOS_INSTALL}/base/mcs/action" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/action"
    chmod -R 640 "${SOPHOS_INSTALL}/base/mcs/action"
    if [ -n "$(ls -A "${SOPHOS_INSTALL}/base/mcs/action" 2>/dev/null)" ]
    then
      rm -rf "${SOPHOS_INSTALL}/base/mcs/action/"*
    fi
fi

if [[ -d "${SOPHOS_INSTALL}/base/mcs/tmp/actions" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/tmp/actions"
    chmod -R 640 "${SOPHOS_INSTALL}/base/mcs/tmp/actions"
    chmod 750 "${SOPHOS_INSTALL}/base/mcs/tmp/actions"
fi

if [[ -d "${SOPHOS_INSTALL}/base/mcs/tmp/policy" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/tmp/policy"
    chmod -R 640 "${SOPHOS_INSTALL}/base/mcs/tmp/policy"
    chmod 750 "${SOPHOS_INSTALL}/base/mcs/tmp/policy"
fi

makedir 750 "${SOPHOS_INSTALL}/base/mcs/action"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/policy"
makedir 700 "${SOPHOS_INSTALL}/base/mcs/tmp"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/response"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/datafeed"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/status"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/status/cache"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/internal_policy"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/event"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/eventcache"
makedir 751 "${SOPHOS_INSTALL}/base/mcs/certs"

makedir 711 "${SOPHOS_INSTALL}/plugins"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/plugins"

chmod 711 "${SOPHOS_INSTALL}/base/mcs"
chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/response"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/datafeed"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/status"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/internal_policy"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/event"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs"

# Remote Diagnose
makedir 751 "${SOPHOS_INSTALL}/base/remote-diagnose/output"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/remote-diagnose"

# Telemetry
makedir 750 "${SOPHOS_INSTALL}/base/telemetry"
makedir 750 "${SOPHOS_INSTALL}/base/telemetry/var"
makedir 770 "${SOPHOS_INSTALL}/base/telemetry/cache"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/telemetry"
if [ -L "${SOPHOS_INSTALL}/opt/sophos-spl/base/bin/versionedcopy" ]
then
    rm -rf "${SOPHOS_INSTALL}/opt/"
fi

## Setup libraries for versionedcopy
INSTALLER_LIB="${SOPHOS_INSTALL}/tmp/install_lib"
export LD_LIBRARY_PATH="$DIST/files/base/lib64:${INSTALLER_LIB}"
mkdir -p "${INSTALLER_LIB}"
ln -snf "${DIST}/files/base/lib64/libstdc++.so."* "${INSTALLER_LIB}/libstdc++.so.6"

chmod u+x "$DIST/files/base/bin"/*

# generate machine id if necessary
"$DIST/files/base/bin/machineid" "${SOPHOS_INSTALL}"

CLEAN_INSTALL=1
# update_report.json is only written after the update has finished and still exists on a downgrade
if ls "${SOPHOS_INSTALL}/base/update/var/updatescheduler/update_report"* 1> /dev/null 2>&1; then
    CLEAN_INSTALL=0
fi


generate_manifest_diff "${DIST}" ${PRODUCT_LINE_ID} || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to generate manifest diff"

find "$DIST/files" -type f -print0 | xargs -0 "$DIST/files/base/bin/versionedcopy" || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy files to installation"

#copy flags-warehouse file into place
FLAGS_SRC="${DIST}/sspl_flags/files/base/etc/sophosspl/flags-warehouse.json"
if [[ -f "${FLAGS_SRC}" ]]
then
  FLAGS_WAREHOUSE_INST="${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json"
  cp "${FLAGS_SRC}" "${FLAGS_WAREHOUSE_INST}"
  chmod 640 "${FLAGS_WAREHOUSE_INST}"
  chown "root:${GROUP_NAME}" "${FLAGS_WAREHOUSE_INST}"
fi

# create a directory which will be used by 3rd party applications with execute permissions for all.
makedir 711 "${SOPHOS_INSTALL}/shared"

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}"/base/etc/telemetry-config.json*
chmod 440 "${SOPHOS_INSTALL}/base/etc/telemetry-config.json"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/bin"
chmod u+x "${SOPHOS_INSTALL}/base/bin"/*
chmod u+x "${SOPHOS_INSTALL}/bin"/*
chmod u+x "${SOPHOS_INSTALL}/base/lib64"/*
chown -h root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/lib64"/*
chmod g+r "${SOPHOS_INSTALL}/base/lib64"/*
chmod o+r "${SOPHOS_INSTALL}/base/lib64/libcrypto.so"*

@INSTALL_ADJUST@

chmod 700 "${SOPHOS_INSTALL}/bin/uninstall.sh."*
chmod 700 "${SOPHOS_INSTALL}/bin/version"*

if [ ! -f "${SOPHOS_INSTALL}/base/etc/logger.conf.local" ]
then
  if [[ -n "$SOPHOS_LOG_LEVEL" ]]
  then
      cat <<EOF >"${SOPHOS_INSTALL}/base/etc/logger.conf.local"
[global]
VERBOSITY = $SOPHOS_LOG_LEVEL
EOF
  else
      touch "${SOPHOS_INSTALL}/base/etc/logger.conf.local"
  fi
fi

chown "root:root" "${SOPHOS_INSTALL}/base/etc/logger.conf" "${SOPHOS_INSTALL}/base/etc/logger.conf.local"
chmod 644 "${SOPHOS_INSTALL}/base/etc/logger.conf"* "${SOPHOS_INSTALL}/base/etc/logger.conf.local"*

chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/datafeed-config-scheduled_query.json"

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/mcsrouter"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/mcsrouter"*
chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/sdu"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/sdu"*

chown -R "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/lib"
chmod -R 750 "${SOPHOS_INSTALL}/base/lib"
chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/python3"*
chmod 710 "${SOPHOS_INSTALL}/base/bin/python3"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/telemetry"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/telemetry"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/tscheduler"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/tscheduler"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/UpdateScheduler"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/UpdateScheduler"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/machineid"*
chmod 710 "${SOPHOS_INSTALL}/base/bin/machineid"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs/"*
chmod go+r "${SOPHOS_INSTALL}/base/mcs/certs/"*

chmod 700 "${SOPHOS_INSTALL}/base/update/versig."*

# Telemetry needs to be able to access the version file.
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/VERSION.ini"
chmod 640 "${SOPHOS_INSTALL}/base/VERSION.ini"

if [[ -n ${https_proxy} ]]
then
    echo -n "${https_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${HTTPS_PROXY} ]]
then
    echo -n "${HTTPS_PROXY}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${http_proxy} ]]
then
    echo -n "${http_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
fi

unset LD_LIBRARY_PATH
cleanup_comms_component

for F in "$DIST/installer/plugins"/*
do
    if changed_or_added "${F#"$DIST"/}" "${DIST}" ${PRODUCT_LINE_ID}
    then
       "${SOPHOS_INSTALL}/bin/wdctl" copyPluginRegistration "$F" || failure ${EXIT_FAIL_WDCTL_FAILED_TO_COPY} "Failed to copy registration $F"
    fi
done

rm -rf "${INSTALLER_LIB}"

EXIT_CODE=0
if (( $CLEAN_INSTALL == 1 ))
then

    echo "Installation complete, performing post install steps"

    # Save the install options file to etc for the product to use
    # INSTALL_OPTIONS_FILE is an env variable passed in by thininstaller
    if [[ -f "$INSTALL_OPTIONS_FILE" ]]
    then
      BASE_INSTALL_OPTIONS_FILE="$SOPHOS_INSTALL/base/etc/install_options"
      cp "$INSTALL_OPTIONS_FILE" "$BASE_INSTALL_OPTIONS_FILE"
      chown "${USER_NAME}:${GROUP_NAME}" "$BASE_INSTALL_OPTIONS_FILE"
      chmod 440 "$BASE_INSTALL_OPTIONS_FILE"
    fi
    generate_local_user_group_id_config

    if [[ -n "$MCS_CA" ]]
    then
        if [[ "$ALLOW_OVERRIDE_MCS_CA" == --allow-override-mcs-ca ]]
        then
            touch "${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
            chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
            chmod 640 "${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
        fi
        export MCS_CA
    fi

    # if this file exists we should use it instead of registering
    if [[ -f ${PREREGISTED_MCS_CONFIG} ]]
    then
        mv "${PREREGISTED_MCS_CONFIG}" "${SOPHOS_INSTALL}/base/etc/mcs.config"
        chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/mcs.config"
        chmod 640 "${SOPHOS_INSTALL}/base/etc/mcs.config"
        if [[ -f ${PREREGISTED_MCS_POLICY_CONFIG} ]]
        then
            mv "${PREREGISTED_MCS_POLICY_CONFIG}" "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
            chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
            chmod 640 "${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config"
        fi
    elif [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" ]]
    then
        "${SOPHOS_INSTALL}"/base/bin/registerCentral "$MCS_TOKEN" "$MCS_URL" $CUSTOMER_TOKEN_ARGUMENT $MCS_MESSAGE_RELAYS $PRODUCT_ARGUMENTS
        REGISTER_EXIT=$?
        if [[ "$REGISTER_EXIT" != 0 ]]
        then
            EXIT_CODE=${EXIT_FAIL_REGISTER}
            echo "Failed to register with Sophos Central: $REGISTER_EXIT"
        fi
    fi
fi

if changed_or_added install.sh "${DIST}" ${PRODUCT_LINE_ID}
then
    createUpdaterSystemdService
    createDiagnoseSystemdService
    createWatchdogSystemdService
fi

if (( $CLEAN_INSTALL == 1 ))
then
    perform_cleanup "${DIST}" ${PRODUCT_LINE_ID}

    startSsplService
    waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"

    if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" && "$EXIT_CODE" == "0" ]]
    then
        waitForProcess "mcsrouter.mcs_router" || failure ${EXIT_FAIL_SERVICE} "MCS Router not running"
    fi
    # Provide time to Watchdog to start all managed services
    sleep 2
elif software_changed "${DIST}" ${PRODUCT_LINE_ID}
then
    if [[ "${SOPHOS_INHIBIT_WATCHDOG_RESTART}" != "true" ]]
    then
      stopSsplService

      perform_cleanup "${DIST}" ${PRODUCT_LINE_ID}

      startSsplService

      waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"

      # Provide time to Watchdog to start all managed services
      sleep 2
    else
      perform_cleanup "${DIST}" ${PRODUCT_LINE_ID}
    fi
fi

copy_manifests "${DIST}" ${PRODUCT_LINE_ID}
echo "managementagent,${PRODUCT_LINE_ID}" >> "${SOPHOS_INSTALL}/base/update/var/installedComponentTracker"

## Exit with error code if registration was run and failed
exit ${EXIT_CODE}
