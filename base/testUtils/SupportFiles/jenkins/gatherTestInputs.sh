#!/usr/bin/env bash
set -exu

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

sudo rm -rf /tmp/system-product-test-inputs/

TEST_PACKAGE_XML=system-product-test-release-package.xml
if [[ -n ${BASE_COVERAGE-} ]]; then
  TEST_PACKAGE_XML=system-product-test-base-coverage.xml
elif [[ -n ${MDR_COVERAGE-} ]]; then
  TEST_PACKAGE_XML=system-product-test-mdr-coverage.xml
fi

# Create venv
# undo set -eu because venv/bin/activate script produces errors.
set +eu
python3 -m venv /tmp/venv-for-ci
source /tmp/venv-for-ci/bin/activate
  $TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh || fail "Error: Failed to get CI scripts"
  export BUILD_JWT=$(cat $TEST_UTILS/SupportFiles/jenkins/jwt_token.txt)
  python3 -m build_scripts.artisan_fetch $TEST_UTILS/$TEST_PACKAGE_XML || fail "Error: Failed to fetch inputs"
deactivate
# restore bash strictness (for scripts that source this one)
set -eu
