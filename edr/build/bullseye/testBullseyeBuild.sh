#!/usr/bin/env bash

## Run the system tests with the output from a build

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

#marked out
[[ -n ${TEST_SELECTOR} ]] || TEST_SELECTOR=

if [[ -n "$COVFILE" ]]
then
    echo "Creating links for COVFILE $COVFILE"
    sudo ln -nsf "$COVFILE" /root/fulltest.cov
    sudo ln -nsf "$COVFILE" /test.cov

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

PRIVATE_KEY="${SCRIPT_DIR}/private.key"
chmod 600 "${PRIVATE_KEY}"
export GIT_SSH_COMMAND="ssh -i ${PRIVATE_KEY}"

#SYSTEM_TEST_CHECKOUT="/tmp/system-tests"


unset LD_LIBRARY_PATH

ln -nsf "$COVFILE" test.cov
ln -nsf "$COVFILE" .

DEVBFR=NOT_FOUND
[[ -d /mnt/filer6/bfr/sspl-base ]] && DEVBFR=/mnt/filer6/bfr
[[ -d /uk-filer6/bfr/sspl-base ]] && DEVBFR=/uk-filer6/bfr

# Use the bullseye build of the EDR plugin which has just been done
SSPL_EDR_PLUGIN="${BASE}/output"
if [[ ! -d ${SSPL_EDR_PLUGIN} ]]
then
    failure 79 "No EDR plugin build"
fi

#move the build to /opt/test/inputs/edr as expected by test is running on TAP
PYTEST_SCRIPTS=/opt/test/inputs/test_scripts
sudo ln -nsf "${BASE}/TA" ${PYTEST_SCRIPTS}
sudo ln -nsf ${SSPL_EDR_PLUGIN}  /opt/test/inputs/edr

## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,BASE_DIST,COVFILE,BASE,
cd ${PYTEST_SCRIPTS}
sudo \
    --preserve-env="${PRESERVE_ENV}" \
    python3 -m pytest

echo "Tests exited with $?"
exit 0
