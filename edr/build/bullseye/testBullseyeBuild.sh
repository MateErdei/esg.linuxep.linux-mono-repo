#!/usr/bin/env bash

## Run the component tests with the output from a coverage build
LOCAL_JENKINS_TEST=0
function failure()
{
    local E=$1
    shift
    echo "$@"
    exit $E
}

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")

[[ -n ${BASE} ]] || BASE=${SCRIPT_DIR}/../..
export BASE

if [[ -n "$COVFILE" ]]
then
    echo "Creating links for COVFILE $COVFILE"
    COVDIR=$(dirname "$COVFILE")
    echo "COVFILE=$COVFILE" >/tmp/BullseyeCoverageEnv.txt
    echo "COVDIR=$COVDIR" >>/tmp/BullseyeCoverageEnv.txt
    sudo chmod 0644 /tmp/BullseyeCoverageEnv.txt
    sudo chmod 0666 "$COVFILE"
    sudo chmod a+x "$COVDIR"
else
    failure 78 "No COVFILE specified"
fi

export BULLSEYE_UPLOAD=1
export COVFILE

COMPONENT_TEST_INPUTS_DIR=/opt/test/inputs
PYTESTDIR=${COMPONENT_TEST_INPUTS_DIR}/test_scripts/

#if running this on local jenkins map the output
if [[ ${LOCAL_JENKINS_TEST} != 0 ]]
then
  export LOCAL_JENKINS_TEST
  PYTESTDIR=${BASE}/TA/
  # Use the bullseye build of the EDR plugin which has just been done
  export SSPL_EDR_PLUGIN_SDDS="$BASE/output/SDDS-COMPONENT"
  if [[ ! -d "$SSPL_EDR_PLUGIN_SDDS" ]]
  then
      failure 79 "No EDR plugin build"
  fi

  sudo mkdir -p ${COMPONENT_TEST_INPUTS_DIR}
  sudo ln -nsf ${BASE}/output ${COMPONENT_TEST_INPUTS_DIR}/edr

## Find mdr component suite
## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,COVFILE,BASE,BULLSEYE_UPLOAD
LOG_LEVEL=TRACE

cd ${PYTESTDIR}
sudo \
        --preserve-env="${PRESERVE_ENV}" \
        python3 -m pytest

[[ ${LOCAL_JENKINS_TEST} != 0 ]] || sudo unlink ${COMPONENT_TEST_INPUTS_DIR}/edr </dev/null

echo "Tests exited with $?"
exit 0