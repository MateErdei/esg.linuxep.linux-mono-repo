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

INPUTS_ROOT=$BASE/../..
AV_ROOT=${INPUTS_ROOT}/av
[[ ! -d $AV_ROOT/INSTALL-SET ]] && AV_ROOT=${INPUTS_ROOT}/output
[[ ! -d $AV_ROOT/INSTALL-SET ]] && AV_ROOT=/opt/test/inputs/av
TEST_SUITE=${BASE}/..

export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-sha1.pem}

SDDS_BASE=${AV_ROOT}/base-sdds
[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"
SDDS_AV=${AV_ROOT}/INSTALL-SET
[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 2 "Can't find $SDDS_AV/install.sh"

SOPHOS_INSTALL=/opt/sophos-spl

## Install Base
"${SDDS_BASE}/install.sh"

## Install AV
"${SDDS_AV}/install.sh"

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
