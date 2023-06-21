#!/usr/bin/env bash
set -ex

function fail {
local msg=${1:-"ERROR"}
echo $msg 1>&2
exit 1
}

function try_command_with_backoff()
{
    for i in {1..5};
    do
      "$@"
      if [[ $? != 0 ]]
      then
        EXIT_CODE=1
        sleep 15
      else
        EXIT_CODE=0
        break
      fi
    done
    return $EXIT_CODE
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
VENV=/tmp/venv-for-ci-tools
set +e
[[ -d "${VENV}" ]] || python3 -m venv "${VENV}"
source "${VENV}/bin/activate" || exit 11
  "$TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh" || fail 'Error: Failed to get CI scripts'
  export BUILD_JWT=$(cat "$TEST_UTILS/SupportFiles/jenkins/jwt_token.txt")
  python3 -m build_scripts.artisan_fetch -m "$MODE" "$TEST_UTILS/$TEST_PACKAGE_XML" || fail "Error: Failed to fetch inputs"
  python3 "$TEST_UTILS/libs/GatherReleaseWarehouses.py" --dest=${SYSTEMPRODUCT_TEST_INPUT} --dogfood-override=${SDDS3_DOGFOOD} --current-shipping-override=${SDDS3_CURRENT_SHIPPING} 2>&1 | tee ${SYSTEMPRODUCT_TEST_INPUT}/GatherReleaseWarehouses.log
deactivate
chmod +x ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/sdds3-builder || fail "Error: Failed to chmod sdds3-builder inputs"
unzip -o -d ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/ ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/safestore-linux-x64.zip
chmod +x ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/ssr/ssr || fail "Error: Failed to chmod safestore tool"
try_command_with_backoff cp -rv /mnt/filer6/linux/SSPL/testautomation/sdds-specs $SYSTEMPRODUCT_TEST_INPUT/sdds-specs || fail "Error: Failed to fetch inputs"
# restore bash strictness (for scripts that source this one)
set -e
