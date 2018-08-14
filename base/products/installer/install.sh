#!/usr/bin/env bash

EXIT_FAIL_CREATE_DIRECTORY=10
EXIT_FAIL_FIND_GROUPADD=11
EXIT_FAIL_ADD_GROUP=12
EXIT_FAIL_FIND_USERADD=13
EXIT_FAIL_ADDUSER=14
EXIT_FAIL_FIND_GETENT=15
EXIT_FAIL_WDCTL_FAILED_TO_COPY=16
EXIT_FAIL_NOT_ROOT=17
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

function createWatchdogSystemdService()
{

    if [[ -d /lib/systemd/system ]]
    then
        STARTUP_DIR="/lib/systemd/system"
    elif [[ -d /usr/lib/systemd/system ]]
    then
        STARTUP_DIR="/usr/lib/systemd/system"
    else
        failure ${EXIT_FAIL_SERVICE} "Could not install the sophos-spl service"
    fi

    #TODO LD_LIBRARY_PATH is being set here until we have a SUL build with rpath set.
    cat > ${STARTUP_DIR}/sophos-spl.service << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}" "LD_LIBRARY_PATH=${SOPHOS_INSTALL}/base/lib64"
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
}

function createUpdaterSystemdService()
{

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

    #TODO LD_LIBRARY_PATH is being set here until we have a SUL build with rpath set.
    cat > ${STARTUP_DIR}/${service_name} << EOF
[Service]
Environment="SOPHOS_INSTALL=${SOPHOS_INSTALL}" "LD_LIBRARY_PATH=${SOPHOS_INSTALL}/base/lib64"
ExecStart=${SOPHOS_INSTALL}/base/bin/SulDownloader ${SOPHOS_INSTALL}/base/update/var/config.json ${SOPHOS_INSTALL}/base/update/var/report.json
Restart=no
Type=oneshot

[Unit]
Description=Sophos Server Protection Update Service
RequiresMountsFor=${SOPHOS_INSTALL}
EOF
    chmod 644 ${STARTUP_DIR}/${service_name}
    systemctl daemon-reload
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

mkdir -p $SOPHOS_INSTALL || failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to create installation directory: $SOPHOS_INSTALL"
chmod 711 "$SOPHOS_INSTALL"
chown root:${GROUP_NAME} "$SOPHOS_INSTALL"

USER_NAME=sophos-spl-user
USERADD="$(which useradd)"
[[ -x "${USERADD}" ]] || USERADD=/usr/sbin/useradd
[[ -x "${USERADD}" ]] || failure ${EXIT_FAIL_FIND_USERADD} "Failed to find useradd to add low-privilege user"
"${GETENT}" passwd "${USER_NAME}" 2>&1 > /dev/null || "${USERADD}" -d "${SOPHOS_INSTALL}" -g "${GROUP_NAME}" -M -N -r -s /bin/false "${USER_NAME}" \
    || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $USER_NAME"

mkdir -p "${SOPHOS_INSTALL}/tmp"
chmod 1770 "${SOPHOS_INSTALL}/tmp"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp"

mkdir -p "$SOPHOS_INSTALL/var/ipc/plugins"
chmod 711 "$SOPHOS_INSTALL/var"
chmod 700 "$SOPHOS_INSTALL/var/ipc"
chmod 700 "$SOPHOS_INSTALL/var/ipc/plugins"
chown "${USER_NAME}:${GROUP_NAME}" "$SOPHOS_INSTALL/var/ipc"
chown "${USER_NAME}:${GROUP_NAME}" "$SOPHOS_INSTALL/var/ipc/plugins"

mkdir -p "${SOPHOS_INSTALL}/logs/base/sophosspl"
chmod 711 "${SOPHOS_INSTALL}/logs"
chmod 711 "${SOPHOS_INSTALL}/logs/base"
chmod 770 "${SOPHOS_INSTALL}/logs/base/sophosspl"
chown "root:" "${SOPHOS_INSTALL}/logs/base"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base/sophosspl"

mkdir -p "${SOPHOS_INSTALL}/base/etc"
chmod 711 "${SOPHOS_INSTALL}/base/etc"

mkdir -p "${SOPHOS_INSTALL}/base/pluginRegistry"


mkdir -p "${SOPHOS_INSTALL}/base/update/cache/primary"
mkdir -p "${SOPHOS_INSTALL}/base/update/cache/primarywarehouse"
mkdir -p "${SOPHOS_INSTALL}/base/update/certs"
mkdir -p "${SOPHOS_INSTALL}/base/update/var"
chmod 700 "${SOPHOS_INSTALL}/base/update/cache/primary"
chmod 700 "${SOPHOS_INSTALL}/base/update/cache/primarywarehouse"
chmod 700 "${SOPHOS_INSTALL}/base/update/certs"
chmod 700 "${SOPHOS_INSTALL}/base/update/var"

mkdir -p "${SOPHOS_INSTALL}/base/mcs/action"
mkdir -p "${SOPHOS_INSTALL}/base/mcs/policy"
mkdir -p "${SOPHOS_INSTALL}/base/mcs/status/cache"
mkdir -p "${SOPHOS_INSTALL}/base/mcs/event"
mkdir -p "${SOPHOS_INSTALL}/base/mcs/certs"
mkdir -p "${SOPHOS_INSTALL}/base/mcs/tmp"
chmod -R 750 "${SOPHOS_INSTALL}/base"
chown -R "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/mcs"

## Setup libraries for versionedcopy
INSTALLER_LIB="${SOPHOS_INSTALL}/tmp/install_lib"
export LD_LIBRARY_PATH="$DIST/files/base/lib64:${INSTALLER_LIB}"
mkdir -p "${INSTALLER_LIB}"
ln -snf "${DIST}/files/base/lib64/libstdc++.so."* "${INSTALLER_LIB}/libstdc++.so.6"

for F in $(find "$DIST/files" -type f)
do
    "$DIST/files/base/bin/versionedcopy" "$F" || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy $F to installation"
done

ln -snf "liblog4cplus-1.2.so" "${SOPHOS_INSTALL}/base/lib64/liblog4cplus.so"

chmod u+x "${SOPHOS_INSTALL}/base/bin"/*
chmod u+x "${SOPHOS_INSTALL}/bin"/*
chmod u+x "${SOPHOS_INSTALL}/base/lib64"/*
chmod 700 "$SOPHOS_INSTALL/base/bin/uninstall.sh."*

chmod 700 "${SOPHOS_INSTALL}/base/update/versig."*

for F in "$DIST/installer/plugins"/*
do
    "${SOPHOS_INSTALL}/bin/wdctl" copyPluginRegistration "$F" || failure ${EXIT_FAIL_WDCTL_FAILED_TO_COPY} "Failed to copy registration $F"
done

chmod 750 "${SOPHOS_INSTALL}/base/pluginRegistry"
chmod 640 "${SOPHOS_INSTALL}/base/pluginRegistry"/*
chown -R "root:${GROUP_NAME}" "${SOPHOS_INSTALL}/base/pluginRegistry"

rm -rf "${INSTALLER_LIB}"

if [[ -n "$MCS_CA" ]]
then
    export MCS_CA
fi
if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" ]]
then
    ${SOPHOS_INSTALL}/base/bin/registerCentral "$MCS_TOKEN" "$MCS_URL" || failure ${EXIT_FAIL_REGISTER} "Failed to register with Sophos Central: $?"
fi

createUpdaterSystemdService
createWatchdogSystemdService
waitForProcess "${SOPHOS_INSTALL}/base/bin/sophos_managementagent" || failure ${EXIT_FAIL_SERVICE} "Management Agent not running"
if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" ]]
then
    waitForProcess "python -m mcsrouter.mcsrouter" || failure ${EXIT_FAIL_SERVICE} "MCS Router not running"
fi

cp "$DIST/manifest.dat" "${SOPHOS_INSTALL}/base/update/manifest.dat"
