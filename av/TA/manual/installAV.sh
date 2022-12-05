#!/bin/bash

set -ex

while getopts t:u:bfadc: flag
do
    case "${flag}" in
        t) MCS_TOKEN=${OPTARG};;
        u) MCS_URL=${OPTARG};;
        c) MCS_CA=${OPTARG};;
        b) BREAK_UPDATING=true;;
        f) FLAGS=true;;
        a) ALC=true;;
        d) CONFIGURE_DEBUG=true;;
        ?) echo "Error: Invalid option was specified -$OPTARG use -t for token -u for url and -b for breaking updating"
          failure 1;;
    esac
done
shift $((OPTIND -1))
# Allow positional arguments instead
if (( $# == 2 ))
then
    MCS_TOKEN=$1
    MCS_URL=$2
fi

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

# Dev and QA regions combined, avoids having to override for each.
export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-and-qa-combined.pem}

SDDS_BASE=${AV_ROOT}/base-sdds
[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"

SDDS_AV=${AV_ROOT}/INSTALL-SET
PYTHON=${PYTHON:-python3}
${PYTHON} $BASE/createInstallSet.py "$SDDS_AV" "${AV_ROOT}/SDDS-COMPONENT" "${AV_ROOT}/.." || failure 2 "Failed to create install-set: $?"
[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 3 "Can't find $SDDS_AV/install.sh"

SOPHOS_INSTALL=/opt/sophos-spl

# uninstall
if [[ -d ${SOPHOS_INSTALL} ]]
then
    ${SOPHOS_INSTALL}/bin/uninstall.sh --force || rm -rf ${SOPHOS_INSTALL} || failure 4 "Unable to remove old SSPL: $?"
fi

## Install Base
chmod 700 "${SDDS_BASE}/install.sh"
bash "${SDDS_BASE}/install.sh" || failure 5 "Unable to install base SSPL: $?"

if [[ $BREAK_UPDATING ]]
then
  mv /opt/sophos-spl/base/bin/SulDownloader.0 /opt/sophos-spl/base/bin/SulDownloader.bk
  mv /opt/sophos-spl/base/bin/UpdateScheduler.0 /opt/sophos-spl/base/bin/UpdateScheduler.bk
fi

# If not registering with Central, copy policies over
if [[ -z $MCS_URL ]]
then
    if [[ $FLAGS ]]
    then
      cp ${TEST_SUITE}/resources/flags_policy/flags_enabled.json ${SOPHOS_INSTALL}/base/mcs/policy/
    fi

    if [[ $ALC ]]
    then
      cp ${TEST_SUITE}/resources/ALC_Policy.xml ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    fi

    # copy AV on-access enabled policy in place
    cp ${TEST_SUITE}/resources/SAV-2_policy_OA_enabled.xml ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml

    ${SOPHOS_INSTALL}/bin/wdctl stop mcsrouter
fi

if [[ -n ${CONFIGURE_DEBUG} ]]
then
    sed -i -e's/VERBOSITY = INFO/VERBOSITY = DEBUG/' ${SOPHOS_INSTALL}/base/etc/logger.conf
fi

## Setup Dev region MCS
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"

## Install AV
chmod 700 "${SDDS_AV}/install.sh"
bash $INSTALL_AV_BASH_OPTS "${SDDS_AV}/install.sh" || failure 6 "Unable to install SSPL-AV: $?"

cp ${TEST_SUITE}/resources/on_access_local_settings.json ${SOPHOS_INSTALL}/plugins/av/var/

if [[ -n $MCS_URL ]]
then
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
