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
AV_ROOT=/opt/test/inputs/av
AV_INSTALL_SET=${AV_ROOT}/INSTALL-SET

bash $BASE/setupAV.sh

[[ -d $AV_INSTALL_SET ]] || failure 2 "Can't find AV_INSTALL_SET: $AV_INSTALL_SET"
[[ -f $AV_INSTALL_SET/install.sh ]] || failure 3 "Can't find $AV_INSTALL_SET/install.sh"
# Check supplements are present:
[[ -f $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat ]] || failure 3 "Can't find $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat"
[[ -f $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat ]] || failure 3 "Can't find $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat"
[[ -f $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/model/model.dat ]] || failure 3 "Can't find $AV_INSTALL_SET/files/plugins/av/chroot/susi/update_source/model/model.dat"

SOPHOS_INSTALL=/opt/sophos-spl

## Install AV
bash $INSTALL_AV_BASH_OPTS "${AV_INSTALL_SET}/install.sh" || failure 6 "Unable to install SSPL-AV: $?"
