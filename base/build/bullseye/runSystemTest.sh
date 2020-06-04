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
[[ -n ${OUTPUT} ]] || OUTPUT=$BASE/output
export OUTPUT
[[ -n "${SYSTEM_PRODUCT_TEST_OUTPUT}" ]] || SYSTEM_PRODUCT_TEST_OUTPUT=$OUTPUT
export SYSTEM_PRODUCT_TEST_OUTPUT

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

SYSTEM_TEST_CHECKOUT="/tmp/system-tests"


[[ -n ${BULLSEYE_SYSTEM_TEST_BRANCH} ]] || BULLSEYE_SYSTEM_TEST_BRANCH=develop

unset LD_LIBRARY_PATH

if [[ -d "${SYSTEM_TEST_CHECKOUT}/.git" ]]
then
    cd "${SYSTEM_TEST_CHECKOUT}"
    git fetch origin \
            --deepen=1 \
            --update-shallow \
            || failure 83 "Failed to fetch ${BULLSEYE_SYSTEM_TEST_BRANCH}"
    git reset --hard || failure  84 "Failed to reset git branch"
    git checkout "${BULLSEYE_SYSTEM_TEST_BRANCH}" \
            || failure 81 "Failed to checkout required branch"
    git pull \
            || failure 80 "Failed to pull system tests"
else
    git clone \
            --branch "${BULLSEYE_SYSTEM_TEST_BRANCH}" \
            --depth 1 \
            --no-single-branch \
            ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git "${SYSTEM_TEST_CHECKOUT}" \
            || failure 82 "Failed to clone system tests"
    cd "${SYSTEM_TEST_CHECKOUT}"
fi

[[ -f ./tests/__init__.robot ]] || failure 77 "Failed to checkout system tests"

ln -nsf "$COVFILE" test.cov
ln -nsf "$COVFILE" .


DEVBFR=NOT_FOUND
[[ -d /mnt/filer6/bfr/sspl-base ]] && DEVBFR=/mnt/filer6/bfr
[[ -d /uk-filer6/bfr/sspl-base ]] && DEVBFR=/uk-filer6/bfr

LASTGOODBUILD () {
        echo $1/$( cat "$1/lastgoodbuild.txt" )
}

## BRANCH OVERRIDES
# You can override the specific branch to use of any jenkins dev build by providing
# one of the below environment variable variables when this script is called
# If a <repo>_BRANCH variable is given, it will use that specific branch from the jenkins build output on filer6
# If none is given, develop will be assumed

[[ ! -z ${BASE_BRANCH} ]]                 || BASE_BRANCH="develop"
[[ ! -z ${EXAMPLE_PLUGIN_BRANCH} ]]       || EXAMPLE_PLUGIN_BRANCH="develop"
[[ ! -z ${AUDIT_PLUGIN_BRANCH} ]]         || AUDIT_PLUGIN_BRANCH="develop"
[[ ! -z ${EVENT_PROCESSOR_BRANCH} ]]      || EVENT_PROCESSOR_BRANCH="develop"
[[ ! -z ${MDR_PLUGIN_BRANCH} ]]           || MDR_PLUGIN_BRANCH="develop"
[[ ! -z ${MDR_COMPONENT_SUITE_BRANCH} ]]  || MDR_COMPONENT_SUITE_BRANCH="develop"

## Find example plugin
if [[ -d "$EXAMPLEPLUGIN_SDDS" ]]
then
    export EXAMPLEPLUGIN_SDDS
else
    EXAMPLE_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-exampleplugin/${EXAMPLE_PLUGIN_BRANCH}" )/sspl-exampleplugin/*/output/SDDS-COMPONENT)

    if [[ -d ${EXAMPLE_PLUGIN_SOURCE} ]]
    then
        export EXAMPLEPLUGIN_SDDS=${EXAMPLE_PLUGIN_SOURCE}
    fi
fi

## Find audit plugin
if [[ -d "$SSPL_AUDIT_PLUGIN_SDDS" ]]
then
    export SSPL_AUDIT_PLUGIN_SDDS
else
    AUDIT_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-audit/${AUDIT_PLUGIN_BRANCH}" )/sspl-audit/*/output/SDDS-COMPONENT)
    if [[ -d ${AUDIT_PLUGIN_SOURCE} ]]
    then
        export SSPL_AUDIT_PLUGIN_SDDS=${AUDIT_PLUGIN_SOURCE}
    fi
fi

## Find event processor plugin
if [[ -d "$SSPL_PLUGIN_EVENTPROCESSOR_SDDS" ]]
then
    export SSPL_PLUGIN_EVENTPROCESSOR_SDDS
else
    EVENT_PROCESSOR_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-eventprocessor/${EVENT_PROCESSOR_BRANCH}" )/sspl-eventprocessor/*/output/SDDS-COMPONENT)
    if [[ -d ${EVENT_PROCESSOR_SOURCE} ]]
    then
        export SSPL_PLUGIN_EVENTPROCESSOR_SDDS=${EVENT_PROCESSOR_SOURCE}
    fi
fi

## Find mdr plugin
if [[ -d "$SSPL_MDR_PLUGIN_SDDS" ]]
then
    export SSPL_MDR_PLUGIN_SDDS
else
    MDR_PLUGIN_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-control-plugin/${MDR_PLUGIN_BRANCH}" )/sspl-mdr-control-plugin/*/output/SDDS-COMPONENT)
    if [[ -d ${MDR_PLUGIN_SOURCE} ]]
    then
        export SSPL_MDR_PLUGIN_SDDS=${MDR_PLUGIN_SOURCE}
    fi
fi

## Find mdr component suite
if [[ -d "$SDDS_SSPL_MDR_COMPONENT_SUITE" ]]
then
    export SDDS_SSPL_DBOS_COMPONENT
    export SDDS_SSPL_OSQUERY_COMPONENT
    export SDDS_SSPL_MDR_COMPONENT
    export SDDS_SSPL_MDR_COMPONENT_SUITE
else
    MDR_COMPONENT_SUITE_SOURCE=$(echo $( LASTGOODBUILD "$DEVBFR/sspl-mdr-componentsuite/${MDR_COMPONENT_SUITE_BRANCH}" )/sspl-mdr-componentsuite/*/output)
    if [[ -d ${MDR_COMPONENT_SUITE_SOURCE} ]]
    then
        export SDDS_SSPL_MDR_COMPONENT_SUITE="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT-SUITE/"
        export SDDS_SSPL_DBOS_COMPONENT="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-DBOS-COMPONENT/"
        export SDDS_SSPL_OSQUERY_COMPONENT="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-OSQUERY-COMPONENT/"
        export SDDS_SSPL_MDR_COMPONENT="${MDR_COMPONENT_SUITE_SOURCE}/SDDS-SSPL-MDR-COMPONENT/"
    fi
fi

[[ -n "${THIN_INSTALLER_OVERRIDE}" ]] && export THIN_INSTALLER_OVERRIDE

## Requires sudo permissions ##

# Ensure we have robot installed for python3.
sudo -H python3 -m pip install robotframework psutil pyzmq watchdog protobuf ecdsa pycrypto awscli argparse paramiko pycapnp kittyfuzzer katnip

PRESERVE_ENV=OUTPUT,BASE_DIST,COVFILE,BASE,EXAMPLEPLUGIN_SDDS,THIN_INSTALLER_OVERRIDE,SYSTEM_PRODUCT_TEST_OUTPUT,SSPL_AUDIT_PLUGIN_SDDS,SSPL_PLUGIN_EVENTPROCESSOR_SDDS,SDDS_SSPL_DBOS_COMPONENT,SDDS_SSPL_OSQUERY_COMPONENT,SDDS_SSPL_MDR_COMPONENT,SDDS_SSPL_MDR_COMPONENT_SUITE,SSPL_MDR_PLUGIN_SDDS
LOG_LEVEL=TRACE
EXCLUSION="-e MANUAL -e AUDIT_PLUGIN  -e EVENT_PLUGIN -e FUZZ -e TESTFAILURE -e AMAZON_LINUX -e MCS_FUZZ -e CUSTOM_LOCATION "
if [[ -n "${TEST_SELECTOR}" ]]
then
    sudo \
        --preserve-env="${PRESERVE_ENV}" \
        python3 -m robot --loglevel "${LOG_LEVEL}" ${EXCLUSION} --test "${TEST_SELECTOR}" tests
else
    sudo \
        --preserve-env="${PRESERVE_ENV}" \
        python3 -m robot --loglevel "${LOG_LEVEL}" ${EXCLUSION} tests
fi

echo "Tests exited with $?"

scp -i ${PRIVATE_KEY} /tmp/system-tests/log.html upload@allegro.eng.sophos:public_html/bullseye/sspl-systemtest-log.html

exit 0
