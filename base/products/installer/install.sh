#!/usr/bin/env bash

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
[[ -x "${GROUPADD}" ]] || failure 11 "Failed to find groupadd to add low-privilege group"
/usr/bin/getent group "${GROUP_NAME}" 2>&1 > /dev/null || "${GROUPADD}" -r "${GROUP_NAME}"

mkdir -p $SOPHOS_INSTALL || failure 10 "Failed to create installation directory: $SOPHOS_INSTALL"
chmod 711 "$SOPHOS_INSTALL"
chown root:${GROUP_NAME} "$SOPHOS_INSTALL"

for F in $(find $DIST/files -type f)
do
    $DIST/installer/versionedcopy $F
done
chmod 700 "$SOPHOS_INSTALL/uninstall.sh"
