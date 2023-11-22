#!/bin/bash

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)

CA_CERT=$BASE/hmr-qa-sha256.pem
export MCS_CA=/${CA_CERT}
cp ${CA_CERT} /
chmod 644 ${MCS_CA} 

exec bash $BASE/../../SophosSetup.sh --allow-override-mcs-ca "$@"
