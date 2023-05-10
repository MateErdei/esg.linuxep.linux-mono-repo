#!/bin/bash

# Updates an existing SPL install by side loading AV.
# Assumes installAV.sh extra steps have already been done.


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
INPUTS_ROOT=$BASE/../..
AV_ROOT=/opt/test/inputs/av
[[ ! -f $AV_ROOT/SDDS-COMPONENT/install.sh ]] && AV_ROOT=${INPUTS_ROOT}/av
[[ ! -f $AV_ROOT/SDDS-COMPONENT/install.sh ]] && AV_ROOT=${INPUTS_ROOT}/output
[[ -f ${AV_ROOT}/SDDS-COMPONENT/manifest.dat ]] || failure 1 "Can't find SDDS-COMPONENT: ${AV_ROOT}/SDDS-COMPONENT/manifest.dat"
TEST_SUITE=${BASE}/..

SDDS_AV=${AV_ROOT}/INSTALL-SET
PYTHON=${PYTHON:-python3}
${PYTHON} $BASE/createInstallSet.py "$SDDS_AV" "${AV_ROOT}/SDDS-COMPONENT" "${AV_ROOT}/.." || failure 2 "Failed to create install-set: $?"
[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 3 "Can't find $SDDS_AV/install.sh"
# Check supplements are present:
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat"

SOPHOS_INSTALL=/opt/sophos-spl

## Install AV
chmod 700 "${SDDS_AV}/install.sh"
bash $INSTALL_AV_BASH_OPTS "${SDDS_AV}/install.sh" || failure 6 "Unable to install SSPL-AV: $?"
