#!/bin/bash

set -ex

function failure()
{
    local EXIT=$1
    shift
    echo "$@" >&1
    exit "$EXIT"
}

build="$1"
shift
[[ -n $build ]] || failure 1 "No build URL specified"

BASE=/opt/test/inputs/di
mkdir -p "${BASE}"
cd "${BASE}"
wget $build/base/linux_x64_rel/installer.zip -nc -O base.zip || true
wget $build/deviceisolation/linux_x64_rel/installer.zip -nc -O di.zip || true

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

rm -rf base di
mkdir -p base di
cd base
unzip ../base.zip
cd ../di
unzip ../di.zip
cd ../base
export SOPHOS_INSTALL=/opt/sophos-spl
bash ./install.sh || failure 5 "Unable to install base SSPL: $?"


if [[ $BREAK_UPDATING ]]
then
  mv /opt/sophos-spl/base/bin/SulDownloader.0 /opt/sophos-spl/base/bin/SulDownloader.bk
fi

## Setup Dev/QA region MCS
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"


cd ../di

## Install Component
bash $INSTALL_COMPONENT_BASH_OPTS "./install.sh" || failure 6 "Unable to install SSPL-DI: $?"

cd ..

function default_mcs_ca()
{
    local MCS_CA

    for MCS_CA in "${TEST_SUITE}/resources/certs/hmr-dev-and-qa-combined.pem" \
        $AV/TA/resources/certs/hmr-dev-and-qa-combined.pem \
        /home/pair/sspl_manual_test_helpers/thininstaller/hmr-qa-sha256.pem \
        ~/sspl_manual_test_helpers/thininstaller/hmr-qa-sha256.pem
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
