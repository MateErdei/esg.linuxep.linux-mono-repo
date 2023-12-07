#!/bin/bash

set -ex

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&1
    exit "$EXIT"
}

function extract_creds()
{
    local SPL_INSTALLER=$1

    MIDDLEBIT=$(awk '/^__MIDDLE_BIT__/ {print NR + 1; exit 0; }' "${SPL_INSTALLER}" </dev/null)
    UC_CERTS=$(awk '/^__UPDATE_CACHE_CERTS__/ {print NR + 1; exit 0; }' "${SPL_INSTALLER}" </dev/null)
    ARCHIVE=$(awk '/^__ARCHIVE_BELOW__/ {print NR + 1; exit 0; }' "${SPL_INSTALLER}" </dev/null)

    # If we have __UPDATE_CACHE_CERTS__ section then the middle section ends there, else it ends at the ARCHIVE marker.
    if [[ -n "${UC_CERTS}" ]]; then
        MIDDLEBIT_SIZE=$((UC_CERTS - MIDDLEBIT - 1))
    else
        MIDDLEBIT_SIZE=$((ARCHIVE - MIDDLEBIT - 1))
    fi
    CREDENTIALS_FILE_PATH=/tmp/credentials.txt
    tail -n+"${MIDDLEBIT}" "${SPL_INSTALLER}" | head -"${MIDDLEBIT_SIZE}" >"${CREDENTIALS_FILE_PATH}"

    MCS_TOKEN=$(sed -ne's/^TOKEN=\(.*\)/\1/p' $CREDENTIALS_FILE_PATH)
    echo MCS_TOKEN
    MCS_URL=$(sed -ne's/^URL=\(.*\)/\1/p' $CREDENTIALS_FILE_PATH)
    echo MCS_URL
}


while getopts e:st:u:bfadc: flag
do
    case "${flag}" in
        t) MCS_TOKEN=${OPTARG};;
        u) MCS_URL=${OPTARG};;
        c) MCS_CA=${OPTARG};;
        b) BREAK_UPDATING=true;;
        f) FLAGS=true;;
        a) ALC=true;;
        d) CONFIGURE_DEBUG=true;;
        e) extract_creds "${OPTARG}"
          ;;
        s)
          export SOPHOS_CORE_DUMP_ON_PLUGIN_KILL=1
          export SOPHOS_ENABLE_CORE_DUMP=1
          ulimit -c unlimited
          ;;
        ?) failure 1 "Error: Invalid option was specified -$OPTARG use -t for token -u for url and -b for breaking updating";;
    esac
done

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

echo BASE=$BASE
INPUTS_ROOT=$BASE/../..
COMPONENT_ROOT=/opt/test/inputs/edr
TEST_SUITE=${BASE}/..

function unpack_zip_if_required()
{
    local ZIP="$1"
    local DEST="$2"
    [[ -f "$ZIP" ]] || return 0
    [[ -d "$DEST" && "$DEST" -nt "$ZIP" ]] && return 0
    unzip "$ZIP" -d "$DEST"
}

SDDS_BASE=${COMPONENT_ROOT}/base-sdds
unpack_zip_if_required "${COMPONENT_ROOT}/base_sdds.zip" "$SDDS_BASE"

[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"

SDDS_COMPONENT=/opt/test/inputs/edr_sdds

[[ -d $SDDS_COMPONENT ]] || failure 1 "Can't find SDDS_COMPONENT: $SDDS_COMPONENT"
[[ -f $SDDS_COMPONENT/install.sh ]] || failure 1 "Can't find SDDS_COMPONENT/install.sh: $SDDS_COMPONENT/install.sh"

SOPHOS_INSTALL=${SOPHOS_INSTALL:-/opt/sophos-spl}

# uninstall
if [[ -d ${SOPHOS_INSTALL} ]]
then
    ${SOPHOS_INSTALL}/bin/uninstall.sh --force || rm -rf ${SOPHOS_INSTALL} || failure 4 "Unable to remove old SSPL: $?"
fi

## Install Base
chmod 700 "${SDDS_BASE}/install.sh"
bash "${SDDS_BASE}/install.sh" || failure 5 "Unable to install base SSPL: $?"

if [[ -n ${CONFIGURE_DEBUG} ]]
then
    sed -i -e's/VERBOSITY = INFO/VERBOSITY = DEBUG/' ${SOPHOS_INSTALL}/base/etc/logger.conf
fi

if [[ $BREAK_UPDATING ]]
then
  mv /opt/sophos-spl/base/bin/SulDownloader.0 /opt/sophos-spl/base/bin/SulDownloader.bk
  # Need to enable feature with warehouse-flags
  if [[ -f ${TEST_SUITE}/resources/flags_policy/flags-warehouse.json ]]
  then
    cp ${TEST_SUITE}/resources/flags_policy/flags-warehouse.json ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
  elif [ -f /vagrant/esg.linuxep.linux-mono-repo/av/TA/resources/flags_policy/flags-warehouse.json ]]
  then
    cp /vagrant/esg.linuxep.linux-mono-repo/av/TA/resources/flags_policy/flags-warehouse.json ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
  fi
fi

## Setup Dev/QA region MCS
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"

## Install Component
chmod 700 "${SDDS_COMPONENT}/install.sh"
bash $INSTALL_EDR_BASH_OPTS "${SDDS_COMPONENT}/install.sh" || failure 6 "Unable to install SSPL-EDR: $?"

function default_mcs_ca()
{
    local MCS_CA

    for MCS_CA in "${TEST_SUITE}/resources/certs/hmr-dev-and-qa-combined.pem" /vagrant/esg.linuxep.linux-mono-repo/av/TA/resources/certs/hmr-dev-and-qa-combined.pem
    do
      if [[ -f "${MCS_CA}" ]]
      then
          echo "${MCS_CA}"
          return
      fi
    done
}

if [[ -n $MCS_URL ]]
then
    # Dev and QA regions combined, avoids having to override for each.
    DEFAULT_MCS_CA=$(default_mcs_ca)
    export MCS_CA=${MCS_CA:-${DEFAULT_MCS_CA}}

    # Register to Central
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
