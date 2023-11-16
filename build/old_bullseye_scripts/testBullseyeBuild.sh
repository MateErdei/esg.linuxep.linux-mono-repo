#!/usr/bin/env bash

## Run the component tests with the output from a coverage build
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

[[ -n ${COVFILE} ]] || COVFILE="/tmp/root/sspl-edr-combined.cov"
if [[ -f "$COVFILE" ]]
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
PYTESTDIR=${BASE}/TA/
# Use the bullseye build of the EDR plugin which has just been done
export SSPL_EDR_PLUGIN_SDDS="$BASE/output/SDDS-COMPONENT"
if [[ ! -d "$SSPL_EDR_PLUGIN_SDDS" ]]
then
    failure 79 "No EDR plugin build"
fi

sudo mkdir -p ${COMPONENT_TEST_INPUTS_DIR}
sudo ln -nsf ${BASE}/output ${COMPONENT_TEST_INPUTS_DIR}/edr

## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,COVFILE,BASE,BULLSEYE_UPLOAD
LOG_LEVEL=TRACE

cd ${PYTESTDIR}
sudo \
        --preserve-env="${PRESERVE_ENV}" \
        python3 -m pytest

sudo unlink ${COMPONENT_TEST_INPUTS_DIR}/edr </dev/null

export COV_HTML_BASE=sspl-plugin-edr-combined
export htmldir=${BASE}/output/coverage/sspl-plugin-edr-combined
bash -x ${BASE}/build/bullseye/uploadResults.sh

echo "Tests exited with $?"
exit 0