#!/usr/bin/env bash
set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

sudo rm -rf /tmp/system-product-test-inputs/

TEST_PACKAGE_XML=system-product-test-release-package.xml
if [[ -n ${BASE_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-base-coverage.xml
elif [[ -n ${MDR_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-mdr-coverage.xml
elif [[ -n ${LIVERESPONSE_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-liveresponse-coverage.xml
elif [[ -n ${EDR_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-edr-coverage.xml
elif [[ -n ${PLUGIN_TEMPLATE_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-plugin-template-coverage.xml
elif [[ -n ${PLUGIN_EVENTJOURNALER_COVERAGE:-} ]]; then
  TEST_PACKAGE_XML=system-product-test-plugin-eventjournaler-coverage.xml
fi

# Create venv
# undo set -eu because venv/bin/activate script produces errors.
VENV=./venv-for-ci
set +e
python3 -m venv "${VENV}" && source "${VENV}/bin/activate"
  "$TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh" || fail 'Error: Failed to get CI scripts'
  export BUILD_JWT=$(cat "$TEST_UTILS/SupportFiles/jenkins/jwt_token.txt")
  python3 -m build_scripts.artisan_fetch "$TEST_UTILS/$TEST_PACKAGE_XML" || fail "Error: Failed to fetch inputs"
  cp -r /mnt/filer6/linux/SSPL/testautomation/sdds-specs/ /tmp/system-product-test-inputs/ || fail "Error: Failed to fetch inputs"
deactivate && rm -rf "${VENV}"
# restore bash strictness (for scripts that source this one)
set -e
