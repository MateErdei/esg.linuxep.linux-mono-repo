#!/bin/bash

set -ex

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&1
    exit "$EXIT"
}

while getopts t:u:bfadc: flag
do
    case "${flag}" in
        t) MCS_TOKEN=${OPTARG};;
        u) MCS_URL=${OPTARG};;
        c) MCS_CA=${OPTARG};;
        b) BREAK_UPDATING=true;;
        d) CONFIGURE_DEBUG=true;;
        ?) failure 1 "Error: Invalid option was specified -$OPTARG use -t for token -u for url and -b for breaking updating";;
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

echo BASE=$BASE
INPUTS_ROOT=$BASE/../..
DI_ROOT=$INPUTS_ROOT
[[ -d $DI_ROOT/device_isolation_sdds ]] || DI_ROOT=/opt/test/inputs
DI_SDDS=$DI_ROOT/device_isolation_sdds
[[ -f "${DI_SDDS}/manifest.dat" ]] || failure 1 "Can't find ${DI_SDDS}/manifest.dat"
TEST_SUITE=${BASE}/..

# Dev and QA regions combined, avoids having to override for each.
export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-and-qa-combined.pem}

SDDS_BASE=${DI_ROOT}/base_sdds
[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"

SOPHOS_INSTALL=/opt/sophos-spl

# uninstall
if [[ -d ${SOPHOS_INSTALL} ]]
then
    ${SOPHOS_INSTALL}/bin/uninstall.sh --force || rm -rf ${SOPHOS_INSTALL} || failure 4 "Unable to remove old SPL: $?"
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

## Install Device Isolation
chmod 700 "${DI_SDDS}/install.sh"
bash $INSTALL_DI_BASH_OPTS "${DI_SDDS}/install.sh" || failure 6 "Unable to install SPL-EJ: $?"

if [[ -n $MCS_URL ]]
then
    # Register to Central
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
