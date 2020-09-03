#!/usr/bin/env bash
set -exu

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

# Create venv
sudo rm -rf /tmp/system-product-test-inputs/
python3 -m venv venv-for-ci
source venv-for-ci/bin/activate
  $TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh || fail "Error: Failed to get CI scripts"
  export BUILD_JWT=$(cat $TEST_UTILS/SupportFiles/jenkins/fake_jwt.txt)
  python3 -m build_scripts.artisan_fetch $TEST_UTILS/system-product-test-release-package.xml || fail "Error: Failed to fetch inputs"
  python3 -m build_scripts.artisan_build $TEST_UTILS/system-product-test-release-package.xml || fail "Error: Failed to set ostia vut address"
deactivate


