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
    #sudo ln -nsf "$COVFILE" /test.cov

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

[[ -n ${BULLSEYE_SYSTEM_TEST_BRANCH} ]] || BULLSEYE_SYSTEM_TEST_BRANCH=master

unset LD_LIBRARY_PATH

#ln -nsf "$COVFILE" test.cov
ln -nsf "$COVFILE" .

DEVBFR=NOT_FOUND
[[ -d /mnt/filer6/bfr/sspl-base ]] && DEVBFR=/mnt/filer6/bfr
[[ -d /uk-filer6/bfr/sspl-base ]] && DEVBFR=/uk-filer6/bfr

LASTGOODBUILD () {
        echo $1/$( cat "$1/lastgoodbuild.txt" )
}

[[ ! -z ${BASE_BRANCH} ]]                 || BASE_BRANCH="master"
[[ ! -z ${EDR_PLUGIN_BRANCH} ]]           || EDR_PLUGIN_BRANCH="master"

## Find base
BASE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-base/${BASE_BRANCH}" )/sspl-base/*/output/SDDS-COMPONENT)
export BASE_DIST=${BASE_SOURCE}


# Use the bullseye build of the EDR plugin which has just been done
SSPL_EDR_PLUGIN="${BASE}/output"
if [[ ! -d ${SSPL_EDR_PLUGIN} ]]
then
    failure 79 "No EDR plugin build"
fi

#move the build to /opt/test/inputs/edr as expected by test is running on TAP
#a) /opt/test/inputs/test_scripts -> /vagrant/sspl-plugin-edr-component/TA
#b) /opt/test/inputs/edr/SDDS-COMPONENT -> /vagrant/sspl-plugin-edr-component/output/SDDS-COMPONENT/
PYTEST_SCRIPTS=/opt/test/inputs/test_scripts
ln -nsf "${BASE}/TA" ${PYTEST_SCRIPTS}
ln -nsf ${SSPL_EDR_PLUGIN}  /opt/test/inputs/edr

## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,BASE_DIST,COVFILE,BASE,
cd ${PYTEST_SCRIPTS}
sudo \
    --preserve-env="${PRESERVE_ENV}" \
    python3 -m pytest

echo "Tests exited with $?"

## Process bullseye output
## upload unit tests
cd $BASE
export BASE
bash -x build/bullseye/uploadResults.sh || exit $?

exit 0
