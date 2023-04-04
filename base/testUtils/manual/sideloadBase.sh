#!/bin/bash

function failure()
{
    echo "$@"
    exit 1
}

set -ex

while getopts t:u:bfadc:r flag
do
    case "${flag}" in
        t) MCS_TOKEN=${OPTARG};;
        u) MCS_URL=${OPTARG};;
        c) MCS_CA=${OPTARG};;
        b) BREAK_UPDATING=true;;
        d) CONFIGURE_DEBUG=true;;
        r) INSTALL_RA=true;;
        ?) failure "Error: Invalid option was specified -$OPTARG use -t for token -u for url and -b for breaking updating"
          ;;
    esac
done

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

export SOPHOS_INSTALL=${SOPHOS_INSTALL:-/opt/sophos-spl}
OUTPUT=${OUTPUT:-/vagrant/esg.linuxep.everest-base/output}
SDDS=${SDDS:-$OUTPUT/SDDS-COMPONENT}

[[ -f $SDDS/install.sh ]] || failure "install.sh not found!"
bash $SDDS/install.sh || failure "Unable to install base SSPL: $?"

if [[ $BREAK_UPDATING ]]
then
  $SOPHOS_INSTALL/bin/wdctl stop UpdateScheduler || true
  mv /opt/sophos-spl/base/bin/SulDownloader.0 /opt/sophos-spl/base/bin/SulDownloader.bk
  mv /opt/sophos-spl/base/bin/UpdateScheduler.0 /opt/sophos-spl/base/bin/UpdateScheduler.bk
fi

if [[ -n ${CONFIGURE_DEBUG} ]]
then
    sed -i -e's/VERBOSITY = INFO/VERBOSITY = DEBUG/' ${SOPHOS_INSTALL}/base/etc/logger.conf
fi

if [[ -n $INSTALL_RA ]]
then
    SDDS_RA=${SDDS_RA:-$OUTPUT/RA-SDDS-COMPONENT}
    [[ -f $SDDS_RA/install.sh ]] || failure "RA install.sh not found!"
    $SDDS_RA/install.sh || failure "Unable to install RA"
fi


# Configure MCS
# Dev and QA regions combined, avoids having to override for each.
export MCS_CA=${MCS_CA:-${BASE}/hmr-dev-and-qa-combined.pem}

## Allow override MCS CA
OVERRIDE_FLAG_FILE="${SOPHOS_INSTALL}/base/mcs/certs/ca_env_override_flag"
touch "${OVERRIDE_FLAG_FILE}"
chown -h "root:sophos-spl-group" "${OVERRIDE_FLAG_FILE}"
chmod 640 "${OVERRIDE_FLAG_FILE}"

if [[ -n $MCS_URL ]]
then
    # Register to Central
    exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
else
    echo "Not registering with Central as no token/url specified"
fi
