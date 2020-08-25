#!/usr/bin/env bash

set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

echo ${WORKSPACE}
echo $@ > ${WORKSPACE}/testUtils/SupportFiles/jenkins/OstiaVUTAddress || fail 1 "Failed to set VUT address"