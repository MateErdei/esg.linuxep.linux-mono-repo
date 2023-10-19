#!/bin/bash

export SOPHOS_INSTALL="${SOPHOS_INSTALL:-/opt/sophos-spl}"
OUTPUT="${OUTPUT:-/vagrant/esg.linuxep.linux-mono-repo/base/output}"
SDDS="${SDDS:-$OUTPUT/RA-SDDS-COMPONENT}"

STARTINGDIR="$(pwd)"
cd "${0%/*}"
BASE="$(pwd)"

set -ex

function failure()
{
    echo "$@"
    exit 1
}

[[ -f "$SDDS/install.sh" ]] || failure "install.sh not found!"
exec "$SDDS/install.sh" "$@"