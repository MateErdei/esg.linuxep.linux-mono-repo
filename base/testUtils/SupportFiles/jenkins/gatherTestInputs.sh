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
MODE=no-suite
if [[ -n ${BASE_COVERAGE:-} ]]; then
  MODE=base-coverage
elif [[ -n ${LIVERESPONSE_COVERAGE:-} ]]; then
  MODE=lr-coverage
elif [[ -n ${EDR_COVERAGE:-} ]]; then
  MODE=edr-coverage
elif [[ -n ${PLUGIN_TEMPLATE_COVERAGE:-} ]]; then
  MODE=template-coverage
elif [[ -n ${PLUGIN_EVENTJOURNALER_COVERAGE:-} ]]; then
  MODE=ej-coverage
elif [[ -n ${FULL_TESTS:-} ]]; then
  MODE=system-test
fi

PYTHONCOMMAND=python3
if [[ -d /home/jenkins ]]
then
  PYTHON38="$(which python3.8)"
  [[ -x "${PYTHON38}" ]] && PYTHONCOMMAND=python3.8
fi
# Create venv
# undo set -eu because venv/bin/activate script produces errors.
VENV=/tmp/venv-for-ci-tools
set +e
[[ -d "${VENV}" ]] || $PYTHONCOMMAND -m venv "${VENV}"
source "${VENV}/bin/activate" || exit 11
  "$TEST_UTILS/SupportFiles/jenkins/SetupCIBuildScripts.sh" || fail 'Error: Failed to get CI scripts'
  export BUILD_JWT=$(cat "$TEST_UTILS/SupportFiles/jenkins/jwt_token.txt")
  $PYTHONCOMMAND -m build_scripts.artisan_fetch -m "$MODE" "$TEST_UTILS/$TEST_PACKAGE_XML" || fail "Error: Failed to fetch inputs"
deactivate
chmod +x ${SYSTEMPRODUCT_TEST_INPUT}/sdds3/sdds3-builder || fail "Error: Failed to chmod sdds3-builder inputs"
unzip -o -d ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/ ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/safestore-linux-x64.zip
chmod +x ${SYSTEMPRODUCT_TEST_INPUT}/safestore_tools/ssr/ssr || fail "Error: Failed to chmod safestore tool"

# restore bash strictness (for scripts that source this one)
set -e
