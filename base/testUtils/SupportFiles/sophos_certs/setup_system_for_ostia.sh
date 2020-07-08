#!/bin/bash -e

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
#exit 1
}

SOURCE=$(dirname "${BASH_SOURCE[0]}")
ROOTCA=${SOURCE}/rootca.crt
PS_ROOTCA=${SOURCE}/ps_rootca.crt
[[ -f ${ROOTCA} ]]  ||  fail  "could not find rootca"
[[ -f ${PS_ROOTCA} ]]  || fail "could not find ps_rootca"

OSTIA_CRT=${SOURCE}/OstiaCA.crt
${SOURCE}/../InstallCertificateToSystem.sh ${OSTIA_CRT} || fail  "failed to install ${OSTIA_CRT}"
CERTS_DIR=/opt/sophos-spl/base/update/certs/
cp -f ${ROOTCA}  ${PS_ROOTCA}  ${CERTS_DIR}  || fail "failed to replace certs in ${CERTS_DIR}"
