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
AV_ROOT=/opt/test/inputs/av
TEST_SUITE=${BASE}/..

function unpack_zip_if_required()
{
    local ZIP="$1"
    local DEST="$2"
    [[ -f "$ZIP" ]] || return
    [[ -d "$DEST" && "$DEST" -nt "$ZIP" ]] && return
    unzip "$ZIP" -d "$DEST"
}

SDDS_BASE=${AV_ROOT}/base-sdds
unpack_zip_if_required "${AV_ROOT}/base_sdds.zip" "$SDDS_BASE"

[[ -d $SDDS_BASE ]] || failure 1 "Can't find SDDS_BASE: $SDDS_BASE"
[[ -f $SDDS_BASE/install.sh ]] || failure 1 "Can't find SDDS_BASE/install.sh: $SDDS_BASE/install.sh"

SDDS_AV=${AV_ROOT}/INSTALL-SET
PYTHON=${PYTHON:-python3}
if [[ -z "$NO_CREATE_INSTALL_SET" || ! -d "$SDDS_AV" ]]
then
    unpack_zip_if_required "${AV_ROOT}/av_sdds.zip" "${AV_ROOT}/SDDS-COMPONENT"
    [[ ! -f $AV_ROOT/SDDS-COMPONENT/install.sh ]] && AV_ROOT=${INPUTS_ROOT}/av
    [[ ! -f $AV_ROOT/SDDS-COMPONENT/install.sh ]] && AV_ROOT=${INPUTS_ROOT}/output
    [[ -f ${AV_ROOT}/SDDS-COMPONENT/manifest.dat ]] || failure 1 "Can't find SDDS-COMPONENT: ${AV_ROOT}/SDDS-COMPONENT/manifest.dat"
    ${PYTHON} $BASE/createInstallSet.py "$SDDS_AV" "${AV_ROOT}/SDDS-COMPONENT" "${AV_ROOT}/.." || failure 2 "Failed to create install-set: $?"
fi

[[ -d $SDDS_AV ]] || failure 2 "Can't find SDDS_AV: $SDDS_AV"
[[ -f $SDDS_AV/install.sh ]] || failure 3 "Can't find $SDDS_AV/install.sh"
# Check supplements are present:
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/vdl/vdl.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/reputation/filerep.dat"
[[ -f $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat ]] || failure 3 "Can't find $SDDS_AV/files/plugins/av/chroot/susi/update_source/model/model.dat"

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
  # Just break SulDownloader - UpdateScheduler running helps make health green
#  mv /opt/sophos-spl/base/bin/UpdateScheduler.0 /opt/sophos-spl/base/bin/UpdateScheduler.bk
  # Need to enable feature with warehouse-flags
  cp ${TEST_SUITE}/resources/flags_policy/flags-warehouse.json ${SOPHOS_INSTALL}/base/etc/sophosspl/flags-warehouse.json
fi

# Dev and QA regions combined, avoids having to override for each.
export MCS_CA=${MCS_CA:-${TEST_SUITE}/resources/certs/hmr-dev-and-qa-combined.pem}

# If not registering with Central, copy policies over
if [[ -z $MCS_URL ]]
then
    if [[ $FLAGS ]]
    then
      cp ${TEST_SUITE}/resources/flags_policy/flags_enabled.json ${SOPHOS_INSTALL}/base/mcs/policy/flags.json
    fi

    if [[ $ALC ]]
    then
      cp ${TEST_SUITE}/resources/ALC_Policy.xml ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
    fi

    # copy AV on-access enabled policy in place
    cp ${TEST_SUITE}/resources/SAV-2_policy_OA_enabled.xml ${SOPHOS_INSTALL}/base/mcs/policy/SAV-2_policy.xml
    cp ${TEST_SUITE}/resources/core_policy/CORE-36_oa_enabled.xml ${SOPHOS_INSTALL}/base/mcs/policy/CORE-36_policy.xml
    cp ${TEST_SUITE}/resources/corc_policy/corc_policy.xml ${SOPHOS_INSTALL}/base/mcs/policy/CORC-37_policy.xml

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

if [[ -n $MCS_URL ]]
then
    # Create override file to force ML on
    touch ${SOPHOS_INSTALL}/plugins/av/chroot/etc/sophos_susi_force_machine_learning
    # Register to Central
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
