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
export COVFILE

# Use the bullseye build of the EDR plugin which has just been done
export SSPL_EDR_PLUGIN_SDDS="$BASE/output/SDDS-COMPONENT"
if [[ ! -d "$SSPL_EDR_PLUGIN_SDDS" ]]
then
    failure 79 "No EDR plugin build"
fi

## Find mdr component suite
## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,BASE_DIST,COVFILE,BASE
LOG_LEVEL=TRACE

COMPONENT_TEST_STAGING_DIR=/opt/test/inputs
sudo mkdir -p ${COMPONENT_TEST_STAGING_DIR}
sudo ln -nsf ${BASE}/output ${COMPONENT_TEST_STAGING_DIR}/edr

PYTESTDIR=${BASE}/TA/
cd ${PYTESTDIR}
sudo \
        --preserve-env="${PRESERVE_ENV}" \
        python3 -m pytest

sudo unlink ${COMPONENT_TEST_STAGING_DIR}/edr
echo "Tests exited with $?"
exit 0