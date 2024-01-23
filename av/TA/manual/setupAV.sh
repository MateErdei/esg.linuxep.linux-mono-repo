#!/bin/bash

# from /vagrant/esg.linuxep.linux-mono-repo copied from build machine
# create install set for AV in /opt/test/inputs/av/INSTALL-SET


STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&1
    exit "$EXIT"
}


echo BASE=$BASE
INPUTS_ROOT="$BASE/../../.."

SDDS_AV="$INPUTS_ROOT/.output/av/linux_x64_rel/installer"

AV_ROOT=/opt/test/inputs/av
INSTALL_SET=${AV_ROOT}/INSTALL-SET

[[ -d "$SDDS_AV" ]] || failure "Failed to find AV SDDS dir"

PYTHON=${PYTHON:-python3}

if [[ -z "$NO_CREATE_INSTALL_SET" || ! -d "$SDDS_AV" ]]
then
    [[ -f "${SDDS_AV}/manifest.dat" ]] || failure 1 "Can't find SDDS-COMPONENT: ${SDDS_AV}/manifest.dat"
    ${PYTHON} $BASE/createInstallSet.py "$INSTALL_SET" "${SDDS_AV}" "${AV_ROOT}/.." || failure 2 "Failed to create install-set: $?"
fi
