#!/bin/bash

function failure()
{
    echo "$@"
    exit 1
}

set -ex

CLEAN=

while getopts t:u:bfadc:rxi: flag
do
    case "${flag}" in
        t) MCS_TOKEN="${OPTARG}";;
        u) MCS_URL="${OPTARG}";;
        c) MCS_CA="${OPTARG}";;
        b) BREAK_UPDATING=true;;
        i) export SOPHOS_INSTALL="${OPTARG}";;
        d)
          CONFIGURE_DEBUG=true
          export SOPHOS_LOG_LEVEL=DEBUG
          ;;
        r) INSTALL_RA=true;;
        x) CLEAN=true;;
        ?) failure "Error: Invalid option was specified -$OPTARG use -t for token -u for url and -b for breaking updating"
          ;;
    esac
done

STARTINGDIR=$(pwd)
cd "${0%/*}"
BASE=$(pwd)

export SOPHOS_INSTALL="${SOPHOS_INSTALL:-/opt/sophos-spl}"
OUTPUT="${OUTPUT:-/vagrant/esg.linuxep.linux-mono-repo/base/output}"
SDDS="${SDDS:-$OUTPUT/SDDS-COMPONENT}"

if [[ -n $CLEAN ]]
then
    if [[ -d "$SOPHOS_INSTALL" ]]
    then
        bash "$SOPHOS_INSTALL/bin/uninstall.sh" --force
    fi
    if [[ -d "/opt/sophos-spl" ]]
    then
        bash "/opt/sophos-spl/bin/uninstall.sh" --force
    fi
fi

if [[ -n "${CONFIGURE_DEBUG}" ]]
then
    export SOPHOS_LOG_LEVEL=DEBUG
fi

[[ -f "$SDDS/install.sh" ]] || failure "install.sh not found!"
bash "$SDDS/install.sh" || failure "Unable to install base SSPL: $?"

if [[ $BREAK_UPDATING ]]
then
  "$SOPHOS_INSTALL/bin/wdctl" stop updatescheduler || true
  mv "${SOPHOS_INSTALL}/base/bin/SulDownloader.0"   "${SOPHOS_INSTALL}/base/bin/SulDownloader.bk"
  mv "${SOPHOS_INSTALL}/base/bin/UpdateScheduler.0" "${SOPHOS_INSTALL}/base/bin/UpdateScheduler.bk"
fi

# Configure MCS
# Dev and QA regions combined, avoids having to override for each.
export MCS_CA="${MCS_CA:-${BASE}/hmr-dev-and-qa-combined.pem}"

if [[ "$MCS_CA" == "${BASE}/hmr-dev-and-qa-combined.pem}" ]]
then
    cp "$MCS_CA" /hmr-dev-and-qa-combined.pem
    export MCS_CA=/hmr-dev-and-qa-combined.pem
fi

## Allow override MCS CA
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"

if [[ -n "$MCS_URL" ]]
then
    # Register to Central
    "${SOPHOS_INSTALL}/base/bin/registerCentral" "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi

if [[ -n "$INSTALL_RA" ]]
then
    SDDS_RA="${SDDS_RA:-$OUTPUT/RA-SDDS-COMPONENT}"
    [[ -f "$SDDS_RA/install.sh" ]] || failure "RA install.sh not found!"
    "$SDDS_RA/install.sh" || failure "Unable to install RA"
fi
