#!/usr/bin/env bash
EXIT_FAIL_CREATE_DIRECTORY=10
EXIT_FAIL_FIND_GROUPADD=11
EXIT_FAIL_ADD_GROUP=12
EXIT_FAIL_FIND_USERADD=13
EXIT_FAIL_ADDUSER=14
EXIT_FAIL_FIND_GETENT=15
EXIT_FAIL_WDCTL_FAILED_TO_COPY=16
EXIT_FAIL_NOT_ROOT=17
EXIT_FAIL_DIR_MARKER=18
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

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

source $ABS_SCRIPTDIR/cleanupinstall.sh

[[ -n "$DIST" ]] || DIST=$ABS_SCRIPTDIR

MCS_TOKEN=${MCS_TOKEN:-}
MCS_URL=${MCS_URL:-}
MCS_MESSAGE_RELAYS=${MCS_MESSAGE_RELAYS:-}
MCS_CA=${MCS_CA:-}

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
            shift
            ALLOW_OVERRIDE_MCS_CA=--allow-override-mcs-ca
            ;;
        *)
            echo "BAD OPTION $1"
            exit 2
            ;;
    esac
    shift
done

function failure()
{
    local CODE=$1
    shift
    echo "$@" >&2
    exit $CODE
}

function isServiceInstalled()
{
    local TARGET="$1"
    systemctl list-unit-files | grep -q "^${TARGET}\b" >/dev/null
}

function createWatchdogSystemdService()
{
    if ! isServiceInstalled sophos-spl.service || $FORCE_INSTALL
    then

        if [[ -d /lib/systemd/system ]]
        then
            STARTUP_DIR="/lib/systemd/system"
        elif [[ -d /usr/lib/systemd/system ]]
        then
            STARTUP_DIR="/usr/lib/systemd/system"
        else
            failure ${EXIT_FAIL_SERVICE} "Could not install the sophos-spl service"
        fi

        cat > ${STARTUP_DIR}/sophos-spl.service << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}"
ExecStart=${SOPHOS_INSTALL}/base/bin/sophos_watchdog
Restart=always

[Install]
WantedBy=multi-user.target

[Unit]
Description=Sophos Linux Protection
RequiresMountsFor=${SOPHOS_INSTALL}
EOF
        chmod 644 ${STARTUP_DIR}/sophos-spl.service
        systemctl daemon-reload
        systemctl enable --quiet sophos-spl.service
        systemctl start sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to start sophos-spl service"
    fi
}

function stopSsplService()
{
    systemctl stop sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to restart sophos-spl service"
}

function startSsplService()
{
    systemctl start sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to restart sophos-spl service"
}

function createUpdaterSystemdService()
{
    if ! isServiceInstalled sophos-spl-update.service || $FORCE_INSTALL
    then
        if [[ -d /lib/systemd/system ]]
        then
            STARTUP_DIR="/lib/systemd/system"
        elif [[ -d /usr/lib/systemd/system ]]
        then
            STARTUP_DIR="/usr/lib/systemd/system"
        else
            failure ${EXIT_FAIL_SERVICE} "Could not install the sophos-spl update service"
        fi
        local service_name="sophos-spl-update.service"

        cat > ${STARTUP_DIR}/${service_name} << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}"
ExecStart=${SOPHOS_INSTALL}/base/bin/SulDownloader ${SOPHOS_INSTALL}/base/update/var/update_config.json ${SOPHOS_INSTALL}/base/update/var/update_report.json
Restart=no

[Unit]
Description=Sophos Server Protection Update Service
RequiresMountsFor=${SOPHOS_INSTALL}
EOF
        chmod 644 ${STARTUP_DIR}/${service_name}
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
    if [[ ! -d $2 ]]
    then
        mkdir -p "$2" ||  failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to create directory: $2"
    fi
    chmod "$1" "$2" ||  failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to apply correct permissions: $1 to directory: $2"
}

function makeRootDirectory()
{
    local install_path=${SOPHOS_INSTALL%/*}

    # Make sure that the install_path string is not empty, in the case of "/foo"
    if [ -z $install_path ]
    then
        install_path="/"
    fi

    while [ ! -z $install_path ] && [ ! -d ${install_path} ]
    do
        install_path=${install_path%/*}
    done

    local createDirs=${SOPHOS_INSTALL#$install_path/}

    #Following loop requires trailing slash
    if  [[ ${createDirs:-1} != "/" ]]
    then
        local createDirs=$createDirs/
    fi

    #Iterate through directories giving minimum execute permissions to allow sophos-spl user to run executables
    while [[ ! -z "$createDirs" ]]
    do
        currentDir=${createDirs%%/*}
        install_path="$install_path/$currentDir"
        makedir 711 $install_path
        createDirs=${createDirs#$currentDir/}
    done
}


if [[ $(id -u) != 0 ]]
then
    failure ${EXIT_FAIL_NOT_ROOT} "Please run this installer as root."
fi

function build_version_less_than_system_version()
{
    test "$(printf '%s\n' ${BUILD_LIBC_VERSION} ${system_libc_version} | sort -V | head -n 1)" != ${BUILD_LIBC_VERSION}
}

function add_group()
{
  local groupname="$1"

  GROUPADD="$(which groupadd)"
  [[ -x "${GROUPADD}" ]] || GROUPADD=/usr/sbin/groupadd
  [[ -x "${GROUPADD}" ]] || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to find groupadd to add low-privilege group"
  "${GETENT}" group "${groupname}" 2>&1 > /dev/null || "${GROUPADD}" -r "${groupname}" || failure ${EXIT_FAIL_ADD_GROUP} "Failed to add group $groupname"
}

function add_user()
{
  local username="$1"
  local groupname="$2"

  USERADD="$(which useradd)"
  [[ -x "${USERADD}" ]] || USERADD=/usr/sbin/useradd
  [[ -x "${USERADD}" ]] || failure ${EXIT_FAIL_FIND_USERADD} "Failed to find useradd to add low-privilege user"
  "${GETENT}" passwd "${username}" 2>&1 > /dev/null || "${USERADD}" -d "${SOPHOS_INSTALL}" -g "${groupname}" -M -N -r -s /bin/false "${username}" \
      || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $username"
}

function versionedcopy_supplement()
{
  local supplement_dir=$1

  if [[ -d "$DIST/$supplement_dir" ]]
  then
    local copy_of_dist=$DIST
    local version_copy="$DIST/files/base/bin/versionedcopy"

    export DIST="$DIST/$supplement_dir"
    find "$DIST/files" -type f -print0 | xargs -0 "$version_copy" || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy files to installation"

    export DIST=${copy_of_dist}
  fi
}

if build_version_less_than_system_version
then
    failure ${EXIT_FAIL_WRONG_LIBC_VERSION} "Failed to install on unsupported system. Detected GLIBC version ${system_libc_version} < required ${BUILD_LIBC_VERSION}"
fi


export DIST
export SOPHOS_INSTALL

## Add a low-privilege group
GROUP_NAME=@SOPHOS_SPL_GROUP@

GETENT=/usr/bin/getent
[[ -x "${GETENT}" ]] || GETENT=$(which getent)
[[ -x "${GETENT}" ]] || failure ${EXIT_FAIL_FIND_GETENT} "Failed to find getent"

add_group "${GROUP_NAME}"

makeRootDirectory "${SOPHOS_INSTALL}"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}"

# Adds a hidden file to mark the install directory which is used by the uninstaller.
touch "${SOPHOS_INSTALL}/.sophos" || failure ${EXIT_FAIL_DIR_MARKER} "Failed to create install directory marker file"

## Add a low-privilege users
USER_NAME=@SOPHOS_SPL_USER@
NETWORK_USER_NAME=@SOPHOS_SPL_NETWORK@
LOCAL_USER_NAME=@SOPHOS_SPL_LOCAL@
add_user "${USER_NAME}" "${GROUP_NAME}"
add_user "${NETWORK_USER_NAME}" "${GROUP_NAME}"
add_user "${LOCAL_USER_NAME}" "${GROUP_NAME}"

makedir 1770 "${SOPHOS_INSTALL}/tmp"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp"

makedir 711 "${SOPHOS_INSTALL}/var"
makedir 700 "${SOPHOS_INSTALL}/var/ipc"
makedir 700 "${SOPHOS_INSTALL}/var/ipc/plugins"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/ipc"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/ipc/plugins"

makedir 770 "${SOPHOS_INSTALL}/var/lock-sophosspl"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/lock-sophosspl"

makedir 700 "${SOPHOS_INSTALL}/var/lock"

makedir 711 "${SOPHOS_INSTALL}/logs"
makedir 711 "${SOPHOS_INSTALL}/logs/base"
makedir 770 "${SOPHOS_INSTALL}/logs/base/sophosspl"
ln -snf "${SOPHOS_INSTALL}/var/sophos-spl-comms/logs" "${SOPHOS_INSTALL}/logs/base/sophos-spl-comms"
chown "root:" "${SOPHOS_INSTALL}/logs/base"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl"

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/mcsrouter.log"
fi

if [[ -f "${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log" ]]
then
    chown "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl/mcs_envelope.log"
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

makedir 711 "${SOPHOS_INSTALL}/base"

makedir 711 "${SOPHOS_INSTALL}/base/etc"
makedir 770 "${SOPHOS_INSTALL}/base/etc/sophosspl"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl"

makedir 750 "${SOPHOS_INSTALL}/base/pluginRegistry"
chown -R "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/pluginRegistry"

makedir 770 "${SOPHOS_INSTALL}/base/update"
makedir 700 "${SOPHOS_INSTALL}/base/update/cache/primary"
makedir 700 "${SOPHOS_INSTALL}/base/update/cache/primarywarehouse"
makedir 770 "${SOPHOS_INSTALL}/base/update/certs"
makedir 770 "${SOPHOS_INSTALL}/base/update/var"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/installedproducts"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/installedproductversions"
makedir 770 "${SOPHOS_INSTALL}/base/update/var/processedReports"
chown "root:${GROUP_NAME}"  "${SOPHOS_INSTALL}/base/update"
chown -R "${USER_NAME}:${GROUP_NAME}"  "${SOPHOS_INSTALL}/base/update/var"
chown -R "${USER_NAME}:${GROUP_NAME}"  "${SOPHOS_INSTALL}/base/update/var/processedReports"
chown -R "${USER_NAME}:${GROUP_NAME}"  "${SOPHOS_INSTALL}/base/update/certs"
chown -R "root:root"  "${SOPHOS_INSTALL}/base/update/var/installedproducts"
chown -R "root:root"  "${SOPHOS_INSTALL}/base/update/var/installedproductversions"


makedir 711 "${SOPHOS_INSTALL}/base/bin"
makedir 711 "${SOPHOS_INSTALL}/base/lib64"

if [[ -d "${SOPHOS_INSTALL}/base/mcs/status" ]]
then
    chmod -R 660 "${SOPHOS_INSTALL}/base/mcs/status"
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
fi

if [[ -d "${SOPHOS_INSTALL}/tmp/actions" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp/actions"
    chmod -R 640 "${SOPHOS_INSTALL}/tmp/actions"
    chmod  750 "${SOPHOS_INSTALL}/tmp/actions"
fi

if [[ -d "${SOPHOS_INSTALL}/tmp/policy" ]]
then
    chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp/policy"
    chmod -R 640 "${SOPHOS_INSTALL}/tmp/policy"
    chmod  750 "${SOPHOS_INSTALL}/tmp/policy"
fi

makedir 750 "${SOPHOS_INSTALL}/base/mcs/action"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/policy"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/response"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/status"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/status/cache"
makedir 770 "${SOPHOS_INSTALL}/base/mcs/event"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/certs"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/tmp"
makedir 750 "${SOPHOS_INSTALL}/var/comms/responses"
makedir 770 "${SOPHOS_INSTALL}/var/comms/requests"
chmod   750 "${SOPHOS_INSTALL}/var/comms"
chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/comms/"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/comms/requests"

makedir 711 "${SOPHOS_INSTALL}/plugins"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/plugins"

chmod 711 "${SOPHOS_INSTALL}/base/mcs"
chown -R "${LOCAL_USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs"

# Telemetry
makedir 750 "${SOPHOS_INSTALL}/base/telemetry"
makedir 750 "${SOPHOS_INSTALL}/base/telemetry/var"
makedir 750 "${SOPHOS_INSTALL}/base/telemetry/cache"
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
[[ -f "${SOPHOS_INSTALL}/base/update/manifest.dat" ]] && mkdir -p "${SOPHOS_INSTALL}/base/update/${PRODUCT_LINE_ID}/" && mv "${SOPHOS_INSTALL}/base/update/manifest.dat"  "${SOPHOS_INSTALL}/base/update/${PRODUCT_LINE_ID}/manifest.dat"
[[ -f "${SOPHOS_INSTALL}/base/update/${PRODUCT_LINE_ID}/manifest.dat" ]] && CLEAN_INSTALL=0

generate_manifest_diff $DIST ${PRODUCT_LINE_ID}

find "$DIST/files" -type f -print0 | xargs -0 "$DIST/files/base/bin/versionedcopy" || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy files to installation"

#Todo LINUXDAR-1939 remove this code when sspl-flags CI build becomes available
#FLAGS_DIST="$DIST/mcsep/flags"
#if [[ -d $FLAGS_DIST ]]
#then
#  cp "$FLAGS_DIST/union.json" "${SOPHOS_INSTALL}/base/etc/sophosspl/features_warehouse.json"
#fi
#Todo LINUXDAR-1939 uncomment this when sspl-flags repo is added to CI build
versionedcopy_supplement "sspl_flags"

FEATURES_WH_FLAGS="${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json"
if [[ -f "${FEATURES_WH_FLAGS}" ]]
then
  chmod 400  -f "${FEATURES_WH_FLAGS}"
  chown "${LOCAL_USER_NAME}:${GROUP_NAME}" -f "${FEATURES_WH_FLAGS}"
fi

ln -snf "liblog4cplus-2.0.so" "${SOPHOS_INSTALL}/base/lib64/liblog4cplus.so"

chown -h "root:${GROUP_NAME}" ${SOPHOS_INSTALL}/base/etc/telemetry-config.json*
chmod 440 ${SOPHOS_INSTALL}/base/etc/telemetry-config.json
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
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/logger.conf"
chmod go+r "${SOPHOS_INSTALL}/base/etc/logger.conf"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/mcsrouter"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/mcsrouter"*

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
chmod g+r "${SOPHOS_INSTALL}/base/mcs/certs/"*

chmod 700 "${SOPHOS_INSTALL}/base/update/versig."*

# Telemetry needs to be able to access the version file.
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/VERSION.ini"
chmod 640 "${SOPHOS_INSTALL}/base/VERSION.ini"

unset LD_LIBRARY_PATH

for F in "$DIST/installer/plugins"/*
do
    if changed_or_added ${F#"$DIST"/} ${DIST} ${PRODUCT_LINE_ID}
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
      chmod 400 "$BASE_INSTALL_OPTIONS_FILE"
    fi

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
    if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" ]]
    then
        ${SOPHOS_INSTALL}/base/bin/registerCentral "$MCS_TOKEN" "$MCS_URL" $MCS_MESSAGE_RELAYS
        REGISTER_EXIT=$?
        if [[ "$REGISTER_EXIT" != 0 ]]
        then
            EXIT_CODE=${EXIT_FAIL_REGISTER}
            echo  "Failed to register with Sophos Central: $REGISTER_EXIT"
        fi
    fi
fi

if changed_or_added install.sh ${DIST} ${PRODUCT_LINE_ID}
then
    createUpdaterSystemdService
    createWatchdogSystemdService
fi

if (( $CLEAN_INSTALL == 1 ))
then
    waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"

    if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != ""  && "$EXIT_CODE" == "0" ]]
    then
        waitForProcess "mcsrouter.mcs_router" || failure ${EXIT_FAIL_SERVICE} "MCS Router not running"
    fi
    # Provide time to Watchdog to start all managed services
    sleep  2
else
    if software_changed ${DIST} ${PRODUCT_LINE_ID}
    then
        stopSsplService

        perform_cleanup ${DIST} ${PRODUCT_LINE_ID}

        startSsplService

        waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"

        # Provide time to Watchdog to start all managed services
        sleep  2
    fi
fi

if [[ -n ${https_proxy} ]]
then
    echo "${https_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${http_proxy} ]]
then
    echo "${http_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
fi

copy_manifests ${DIST} ${PRODUCT_LINE_ID}

## Exit with error code if registration was run and failed
exit ${EXIT_CODE}
