#!/bin/bash

set -ex

[[ -n $MCS_TOKEN ]] || MCS_TOKEN=$1
[[ -n $MCS_URL ]] || MCS_URL=$2

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

export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-sha1.pem}

SDDS_BASE=${AV_ROOT}/base-sdds
[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"

SDDS_AV=${AV_ROOT}/INSTALL-SET
python3 $BASE/createInstallSet.py "$SDDS_AV" "${AV_ROOT}/SDDS-COMPONENT" "${AV_ROOT}/.."
[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 3 "Can't find $SDDS_AV/install.sh"

SOPHOS_INSTALL=/opt/sophos-spl

# uninstall
if [[ -d ${SOPHOS_INSTALL} ]]
then
    ${SOPHOS_INSTALL}/bin/uninstall.sh --force || rm -rf ${SOPHOS_INSTALL} || failure 4 "Unable to remove old SSPL: $?"
fi

## Install Base
"${SDDS_BASE}/install.sh" || failure 5 "Unable to install base SSPL: $?"

## Install AV
"${SDDS_AV}/install.sh" || failure 6 "Unable to install SSPL-AV: $?"

## Setup Dev region MCS
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"

if [[ -n $MCS_URL ]]
then
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
