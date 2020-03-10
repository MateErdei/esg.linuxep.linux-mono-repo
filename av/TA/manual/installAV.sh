#!/bin/bash

set -ex

[[ -n $MCS_TOKEN ]] || MCS_TOKEN=$1
[[ -n $MCS_URL ]] || MCS_URL=$2

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

INPUTS_ROOT=$BASE/../..
AV_ROOT=${INPUTS_ROOT}/av
[[ ! -d $AV_ROOT ]] && AV_ROOT=${INPUTS_ROOT}/output
TEST_SUITE=${BASE}/..

export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-sha1.pem}

SDDS_BASE=${AV_ROOT}/base-sdds
[[ -d $SDDS_BASE ]] || { echo "Can't find SDDS_BASE: $SDDS_BASE" ; exit 1 ; }
SDDS_AV=${AV_ROOT}/SDDS-COMPONENT
[[ -d $SDDS_AV ]] || { echo "Can't find SDDS_AV: $SDDS_AV" ; exit 2 ; }

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

exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
