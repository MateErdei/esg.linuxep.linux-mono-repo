#!/usr/bin/env bash
set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

[[ -z $SYSTEMPRODUCT_TEST_INPUT ]] && SYSTEMPRODUCT_TEST_INPUT=/tmp/system-product-test-inputs
sudo rm -rf $SYSTEMPRODUCT_TEST_INPUT

TEST_PACKAGE_XML=system-product-test-release-package.xml
MODE=system-test
if [[ -n ${BASE_COVERAGE:-} ]]; then
  MODE=base-coverage
elif [[ -n ${MDR_COVERAGE:-} ]]; then
  MODE=mdr-coverage
elif [[ -n ${LIVERESPONSE_COVERAGE:-} ]]; then
  MODE=lr-coverage
elif [[ -n ${EDR_COVERAGE:-} ]]; then
  MODE=edr-coverage
elif [[ -n ${PLUGIN_TEMPLATE_COVERAGE:-} ]]; then
  MODE=template-coverage
elif [[ -n ${PLUGIN_EVENTJOURNALER_COVERAGE:-} ]]; then
  MODE=ej-coverage
fi

# Create venv
# undo set -eu because venv/bin/activate script produces errors.
VENV=./venv-for-ci
set +e
python3 -m venv "${VENV}" && source "${VENV}/bin/activate"
  "$TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh" || fail 'Error: Failed to get CI scripts'
  export BUILD_JWT=$(cat "$TEST_UTILS/SupportFiles/jenkins/jwt_token.txt")
  python3 -m build_scripts.artisan_fetch -m "$MODE" "$TEST_UTILS/$TEST_PACKAGE_XML" || fail "Error: Failed to fetch inputs"
deactivate && rm -rf "${VENV}"
cp -r /mnt/filer6/linux/SSPL/testautomation/sdds-specs $SYSTEMPRODUCT_TEST_INPUT/sdds-specs || fail "Error: Failed to fetch inputs"
# restore bash strictness (for scripts that source this one)
set -e
