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

umask 077

STARTINGDIR=$(pwd)
SCRIPTDIR=${0%/*}
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR=${STARTINGDIR}
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl
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
Description=Sophos Server Protection for Linux
RequiresMountsFor=${SOPHOS_INSTALL}
EOF
        chmod 644 ${STARTUP_DIR}/sophos-spl.service
        systemctl daemon-reload
        systemctl enable --quiet sophos-spl.service
        systemctl start sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to start sophos-spl service"
    fi
}

function restartSsplService()
{
    systemctl restart sophos-spl.service || failure ${EXIT_FAIL_SERVICE} "Failed to restart sophos-spl service"
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
    if [[ ! -d $2 ]]
    then
        mkdir -p "$2" && chmod "$1" "$2" ||  failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to create directory: $2 with permissions: $1"
    fi
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

export DIST
export SOPHOS_INSTALL

## Add a low-privilege group
GROUP_NAME=sophos-spl-group

GETENT=/usr/bin/getent
[[ -x "${GETENT}" ]] || GETENT=$(which getent)
[[ -x "${GETENT}" ]] || failure ${EXIT_FAIL_FIND_GETENT} "Failed to find getent"

GROUPADD="$(which groupadd)"
[[ -x "${GROUPADD}" ]] || GROUPADD=/usr/sbin/groupadd
[[ -x "${GROUPADD}" ]] || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to find groupadd to add low-privilege group"
"${GETENT}" group "${GROUP_NAME}" 2>&1 > /dev/null || "${GROUPADD}" -r "${GROUP_NAME}" || failure ${EXIT_FAIL_ADD_GROUP} "Failed to add group $GROUP_NAME"

makeRootDirectory "${SOPHOS_INSTALL}"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}"

# Adds a hidden file to mark the install directory which is used by the uninstaller.
touch "${SOPHOS_INSTALL}/.sophos" || failure ${EXIT_FAIL_DIR_MARKER} "Failed to create install directory marker file"

USER_NAME=sophos-spl-user
USERADD="$(which useradd)"
[[ -x "${USERADD}" ]] || USERADD=/usr/sbin/useradd
[[ -x "${USERADD}" ]] || failure ${EXIT_FAIL_FIND_USERADD} "Failed to find useradd to add low-privilege user"
"${GETENT}" passwd "${USER_NAME}" 2>&1 > /dev/null || "${USERADD}" -d "${SOPHOS_INSTALL}" -g "${GROUP_NAME}" -M -N -r -s /bin/false "${USER_NAME}" \
    || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $USER_NAME"

makedir 1770 "${SOPHOS_INSTALL}/tmp"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp"

makedir 711 "${SOPHOS_INSTALL}/var"
makedir 700 "${SOPHOS_INSTALL}/var/ipc"
makedir 700 "${SOPHOS_INSTALL}/var/ipc/plugins"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/ipc"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/ipc/plugins"

makedir 700 "${SOPHOS_INSTALL}/var/lock-sophosspl"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/var/lock-sophosspl"

makedir 700 "${SOPHOS_INSTALL}/var/lock"

makedir 711 "${SOPHOS_INSTALL}/logs"
makedir 711 "${SOPHOS_INSTALL}/logs/base"
makedir 770 "${SOPHOS_INSTALL}/logs/base/sophosspl"
chown "root:" "${SOPHOS_INSTALL}/logs/base"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl"

makedir 711 "${SOPHOS_INSTALL}/base"

makedir 711 "${SOPHOS_INSTALL}/base/etc"
makedir 770 "${SOPHOS_INSTALL}/base/etc/sophosspl"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/sophosspl"

makedir 750 "${SOPHOS_INSTALL}/base/pluginRegistry"
chown -R "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/pluginRegistry"

makedir 700 "${SOPHOS_INSTALL}/base/update/cache/primary"
makedir 700 "${SOPHOS_INSTALL}/base/update/cache/primarywarehouse"
makedir 700 "${SOPHOS_INSTALL}/base/update/certs"
makedir 700 "${SOPHOS_INSTALL}/base/update/var"
makedir 700 "${SOPHOS_INSTALL}/base/update/var/installedproducts"

makedir 711 "${SOPHOS_INSTALL}/base/bin"
makedir 711 "${SOPHOS_INSTALL}/base/lib64"

makedir 750 "${SOPHOS_INSTALL}/base/mcs/action"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/policy"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/status/cache"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/event"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/certs"
makedir 750 "${SOPHOS_INSTALL}/base/mcs/tmp"

makedir 711 "${SOPHOS_INSTALL}/plugins"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/plugins"

chmod 711 "${SOPHOS_INSTALL}/base/mcs"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs"
chown "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs"

# Telemetry
makedir 750 "${SOPHOS_INSTALL}/base/telemetry"
makedir 750 "${SOPHOS_INSTALL}/base/telemetry/var"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/telemetry"

## Setup libraries for versionedcopy
INSTALLER_LIB="${SOPHOS_INSTALL}/tmp/install_lib"
export LD_LIBRARY_PATH="$DIST/files/base/lib64:${INSTALLER_LIB}"
mkdir -p "${INSTALLER_LIB}"
ln -snf "${DIST}/files/base/lib64/libstdc++.so."* "${INSTALLER_LIB}/libstdc++.so.6"

chmod u+x "$DIST/files/base/bin"/*

# generate machine id if necessary
"$DIST/files/base/bin/machineid" "${SOPHOS_INSTALL}"

"$DIST/files/base/bin/manifestdiff" \
    --old="${SOPHOS_INSTALL}/base/update/manifest.dat" \
    --new="$DIST/manifest.dat" \
    --added="${SOPHOS_INSTALL}/tmp/addedFiles" \
    --removed="${SOPHOS_INSTALL}/tmp/removedFiles" \
    --diff="${SOPHOS_INSTALL}/tmp/changedFiles"


CLEAN_INSTALL=1
[[ -f "${SOPHOS_INSTALL}/base/update/manifest.dat" ]] && CLEAN_INSTALL=0

function changedOrAdded()
{
    local TARGET="$1"
    grep -q "^${TARGET}\$" "${SOPHOS_INSTALL}/tmp/addedFiles" "${SOPHOS_INSTALL}/tmp/changedFiles" >/dev/null
}

for F in $(find "$DIST/files" -type f)
do
    "$DIST/files/base/bin/versionedcopy" "$F" || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy $F to installation"
done

ln -snf "liblog4cplus-2.0.so" "${SOPHOS_INSTALL}/base/lib64/liblog4cplus.so"

chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base"
chown root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/bin"
chmod u+x "${SOPHOS_INSTALL}/base/bin"/*
chmod u+x "${SOPHOS_INSTALL}/bin"/*
chmod u+x "${SOPHOS_INSTALL}/base/lib64"/*
chown -h root:${GROUP_NAME} "${SOPHOS_INSTALL}/base/lib64"/*
chmod g+r "${SOPHOS_INSTALL}/base/lib64"/*
chmod 700 "${SOPHOS_INSTALL}/bin/uninstall.sh."*
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/etc/logger.conf"

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chmod 710 "${SOPHOS_INSTALL}/base/bin/sophos_managementagent"*
chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/mcsrouter"*
chmod 710 "${SOPHOS_INSTALL}/base/bin/mcsrouter"*

chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/lib"
chown -h "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/lib64/python27.zip"*
chown -h "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/python"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/bin/telemetry"*
chmod 750 "${SOPHOS_INSTALL}/base/bin/telemetry"*

chown -h "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs/certs/"*
chmod g+r "${SOPHOS_INSTALL}/base/mcs/certs/"*

chmod 700 "${SOPHOS_INSTALL}/base/update/versig."*

unset LD_LIBRARY_PATH

for F in "$DIST/installer/plugins"/*
do
    if changedOrAdded ${F#"$DIST"/}
    then
       "${SOPHOS_INSTALL}/bin/wdctl" copyPluginRegistration "$F" || failure ${EXIT_FAIL_WDCTL_FAILED_TO_COPY} "Failed to copy registration $F"
    fi
done

rm -rf "${INSTALLER_LIB}"

EXIT_CODE=0
if (( $CLEAN_INSTALL == 1 ))
then
    if [[ -n "$MCS_CA" ]]
    then
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

if changedOrAdded install.sh
then
    createUpdaterSystemdService
    createWatchdogSystemdService
fi

function ssplChanged()
{
    [ -s "${SOPHOS_INSTALL}/tmp/addedFiles" ] || \
    [ -s "${SOPHOS_INSTALL}/tmp/changedFiles" ] || \
    [ -s "${SOPHOS_INSTALL}/tmp/removedFiles" ]
}

if (( $CLEAN_INSTALL == 1 ))
then
    waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"
    if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != ""  && "EXIT_CODE" == 0 ]]
    then
        waitForProcess "python -m mcsrouter.mcs_router" || failure ${EXIT_FAIL_SERVICE} "MCS Router not running"
    fi
else
    if ssplChanged
    then
        restartSsplService
    fi
fi

if [[ -n ${https_proxy} ]]
then
    echo "${https_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
elif [[ -n ${http_proxy} ]]
then
    echo "${http_proxy}" > "${SOPHOS_INSTALL}/base/etc/savedproxy.config"
fi

cp "$DIST/manifest.dat" "${SOPHOS_INSTALL}/base/update/manifest.dat"
chmod 600 "${SOPHOS_INSTALL}/base/update/manifest.dat"

## Exit with error code if registration was run and failed
exit ${EXIT_CODE}
