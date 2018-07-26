#!/usr/bin/env bash

EXIT_FAIL_CREATE_DIRECTORY=10
EXIT_FAIL_FIND_GROUPADD=11
EXIT_FAIL_ADD_GROUP=12
EXIT_FAIL_FIND_USERADD=13
EXIT_FAIL_ADDUSER=14
EXIT_FAIL_FIND_GETENT=15
EXIT_FAIL_VERSIONEDCOPY=20
EXIT_FAIL_REGISTER=30

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

failure()
{
    local CODE=$1
    shift
    echo "$@" >&2
    exit $CODE
}
export LD_LIBRARY_PATH=$DIST/files/base/lib64
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
"${GETENT}" passwd "${USER_NAME}" || "${USERADD}" -d "${SOPHOS_INSTALL}" -g "${GROUP_NAME}" -M -N -r -s /bin/false "${USER_NAME}" \
    || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $USER_NAME"

for F in $(find $DIST/files -type f)
do
    $DIST/installer/versionedcopy $F || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy $F to installation"
done
install $DIST/installer/versionedcopy  "$SOPHOS_INSTALL/base/bin"

mkdir -p "$SOPHOS_INSTALL/var/ipc/plugins"
chmod 711 "$SOPHOS_INSTALL/var"
chmod 700 "$SOPHOS_INSTALL/var/ipc"
chmod 700 "$SOPHOS_INSTALL/var/ipc/plugins"
chown "${USER_NAME}:${GROUP_NAME}" "$SOPHOS_INSTALL/var/ipc"
chown "${USER_NAME}:${GROUP_NAME}" "$SOPHOS_INSTALL/var/ipc/plugins"

mkdir -p "${SOPHOS_INSTALL}/logs/base"
chmod 711 "${SOPHOS_INSTALL}/logs"
chmod 700 "${SOPHOS_INSTALL}/logs/base"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/logs/base"

mkdir -p "${SOPHOS_INSTALL}/tmp"
chmod 1770 "${SOPHOS_INSTALL}/tmp"
chown "${USER_NAME}:${GROUP_NAME}" "${SOPHOS_INSTALL}/tmp"

chmod u+x "${SOPHOS_INSTALL}/base/bin"/*
chmod u+x "${SOPHOS_INSTALL}/base/lib64"/*

chmod 700 "$SOPHOS_INSTALL/base/bin/uninstall.sh"

mkdir -p "${SOPHOS_INSTALL}/base/etc"
chmod 711 "${SOPHOS_INSTALL}/base/etc"

mkdir -p "${SOPHOS_INSTALL}/base/pluginRegistry"
chmod 711 "${SOPHOS_INSTALL}/base/pluginRegistry"

mkdir -p "${SOPHOS_INSTALL}/base/update/cache/Primary"
mkdir -p "${SOPHOS_INSTALL}/base/update/cache/PrimaryWarehouse"
chmod 711 -R "${SOPHOS_INSTALL}/base/update/cache/"

if [[ -n "$MCS_CA" ]]
then
    export MCS_CA
fi

if [[ "$MCS_URL" != "" && "$MCS_TOKEN" != "" ]]
then
    $SOPHOS_INSTALL/base/bin/registerCentral "$MCS_TOKEN" "$MCS_URL" || failure ${EXIT_FAIL_REGISTER} "Failed to register with Sophos Central: $?"
fi
