#!/usr/bin/env bash

EXIT_FAIL_CREATE_DIRECTORY=10
EXIT_FAIL_FIND_GROUPADD=11
EXIT_FAIL_ADD_GROUP=12
EXIT_FAIL_FIND_USERADD=13
EXIT_FAIL_ADDUSER=14
EXIT_FAIL_VERSIONEDCOPY=20

STARTINGDIR=$(pwd)
SCRIPTDIR=${0%/*}
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR=${STARTINGDIR}
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

[[ -n "$SOPHOS_INSTALL" ]] || SOPHOS_INSTALL=/opt/sophos-spl
[[ -n "$DIST" ]] || DIST=$ABS_SCRIPTDIR

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
GROUPADD="$(which groupadd)"
[[ -x "${GROUPADD}" ]] || GROUPADD=/usr/sbin/groupadd
[[ -x "${GROUPADD}" ]] || failure ${EXIT_FAIL_FIND_GROUPADD} "Failed to find groupadd to add low-privilege group"
/usr/bin/getent group "${GROUP_NAME}" 2>&1 > /dev/null || "${GROUPADD}" -r "${GROUP_NAME}" || failure ${EXIT_FAIL_ADD_GROUP} "Failed to add group $GROUP_NAME"

mkdir -p $SOPHOS_INSTALL || failure ${EXIT_FAIL_CREATE_DIRECTORY} "Failed to create installation directory: $SOPHOS_INSTALL"
chmod 711 "$SOPHOS_INSTALL"
chown root:${GROUP_NAME} "$SOPHOS_INSTALL"

USER_NAME=sophos-spl-user
USERADD="$(which useradd)"
[[ -x "${USERADD}" ]] || USERADD=/usr/sbin/useradd
[[ -x "${USERADD}" ]] || failure ${EXIT_FAIL_FIND_USERADD} "Failed to find useradd to add low-privilege user"
/usr/bin/getent shadow "${USER_NAME}" || "${USERADD}" -d "${SOPHOS_INSTALL}" -g "${GROUP_NAME}" -M -N -r -s /bin/false "${USER_NAME}" \
    || failure ${EXIT_FAIL_ADDUSER} "Failed to add user $USER_NAME"

for F in $(find $DIST/files -type f)
do
    $DIST/installer/versionedcopy $F || failure ${EXIT_FAIL_VERSIONEDCOPY} "Failed to copy $F to installation"
done
chmod 700 "$SOPHOS_INSTALL/uninstall.sh"
