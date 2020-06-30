#!/usr/bin/env bash
set -exu

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

sudo rm -rf /tmp/system-product-test-inputs/

sudo $WORKSPACE/testUtils/SupportFiles/jenkins/SetupCIBuildScripts.sh || fail "Error: Failed to get CI scripts"
pushd $WORKSPACE
python3 -m build_scripts.artisan_fetch $WORKSPACE/testUtils/system-product-test-release-package.xml || fail "Error: Failed to fetch inputs"
popd

