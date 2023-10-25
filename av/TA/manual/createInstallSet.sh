#!/bin/bash

set -ex

cd ${0%/*}
BASE=$(pwd)
AV_ROOT=${AV_ROOT:-/opt/test/inputs/av}
SUPPLEMENT_DIR=${SUPPLEMENT_DIR:-${AV_ROOT}/..}

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&1
    exit "$EXIT"
}

function unpack_zip_if_required()
{
    local ZIP="$1"
    local DEST="$2"
    [[ -f "$ZIP" ]] || return
    [[ -d "$DEST" && "$DEST" -nt "$ZIP" ]] && return
    unzip "$ZIP" -d "$DEST"
}

SDDS_AV=${SDDS_AV:-${AV_ROOT}/INSTALL-SET}
PYTHON=${PYTHON:-python3}
unpack_zip_if_required "${AV_ROOT}/av_sdds.zip" "${AV_ROOT}/SDDS-COMPONENT"
${PYTHON} $BASE/createInstallSet.py "$SDDS_AV" "${AV_ROOT}/SDDS-COMPONENT" "${SUPPLEMENT_DIR}" || failure 2 "Failed to create install-set: $?"

[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 3 "Can't find $SDDS_AV/install.sh"
# Check supplements are present:
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat"
