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


[[ -n ${BULLSEYE_SYSTEM_TEST_BRANCH} ]] || BULLSEYE_SYSTEM_TEST_BRANCH=master

unset LD_LIBRARY_PATH

if [[ -d "${SYSTEM_TEST_CHECKOUT}/.git" ]]
then
    cd "${SYSTEM_TEST_CHECKOUT}"
    git fetch origin \
            --deepen=1 \
            --update-shallow \
            || failure 83 "Failed to fetch ${BULLSEYE_SYSTEM_TEST_BRANCH}"
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

FILER_6_LINUX=NOT_FOUND
[[ -d /mnt/filer6/linux/SSPL ]] && FILER_6_LINUX=/mnt/filer6/linux
[[ -d /uk-filer6/linux/SSPL ]] && FILER_6_LINUX=/uk-filer6/linux

FILER_5_BIR=NOT_FOUND
[[ -d /uk-filer5/prodro/bir/sspl-exampleplugin ]] && FILER_5_BIR=/uk-filer5/prodro/bir
[[ -d /mnt/filer5/prodro/bir/sspl-exampleplugin ]] && FILER_5_BIR=/mnt/filer5/prodro/bir

## Find example plugin
if [[ -d "$EXAMPLEPLUGIN_SDDS" ]]
then
    export EXAMPLEPLUGIN_SDDS
else

    if [[ -d "$FILER_6_LINUX/SSPL/JenkinsBuildOutput/Example/master/SDDS-COMPONENT" ]]
    then
        export EXAMPLEPLUGIN_SDDS=$FILER_6_LINUX/SSPL/JenkinsBuildOutput/Example/master/SDDS-COMPONENT
    elif [[ -d "$FILER_5_BIR/sspl-exampleplugin" ]]
    then
        DIR=$(ls -1 "$FILER_5_BIR/sspl-exampleplugin/0-*/*/output/SDDS-COMPONENT" | sort -rV | head -1)
        if [[ -d "$DIR" ]]
        then
            export EXAMPLEPLUGIN_SDDS="$DIR"
        fi
    fi
fi

## Find audit plugin
if [[ -d "$SSPL_AUDIT_PLUGIN_SDDS" ]]
then
    export SSPL_AUDIT_PLUGIN_SDDS
else

    if [[ -d "$FILER_6_LINUX/SSPL/JenkinsBuildOutput/AuditPlugin/master/SDDS-COMPONENT" ]]
    then
        export SSPL_AUDIT_PLUGIN_SDDS=$FILER_6_LINUX/SSPL/JenkinsBuildOutput/AuditPlugin/master/SDDS-COMPONENT
    elif [[ -d "$FILER_5_BIR/sspl-audit" ]]
    then
        DIR=$(ls -1 "$FILER_5_BIR/sspl-audit/0-*/*/output/SDDS-COMPONENT" | sort -rV | head -1)
        if [[ -d "$DIR" ]]
        then
            export SSPL_AUDIT_PLUGIN_SDDS="$DIR"
        fi
    fi
fi

## Find event processor plugin
if [[ -d "$SSPL_PLUGIN_EVENTPROCESSOR_SDDS" ]]
then
    export SSPL_PLUGIN_EVENTPROCESSOR_SDDS
else

    if [[ -d "$FILER_6_LINUX/SSPL/JenkinsBuildOutput/EventProcessor/master/SDDS-COMPONENT" ]]
    then
        export SSPL_PLUGIN_EVENTPROCESSOR_SDDS=$FILER_6_LINUX/SSPL/JenkinsBuildOutput/EventProcessor/master/SDDS-COMPONENT
    elif [[ -d "$FILER_5_BIR/sspl-eventprocessor" ]]
    then
        DIR=$(ls -1 "$FILER_5_BIR/sspl-eventprocessor/0-*/*/output/SDDS-COMPONENT" | sort -rV | head -1)
        if [[ -d "$DIR" ]]
        then
            export SSPL_PLUGIN_EVENTPROCESSOR_SDDS="$DIR"
        fi
    fi
fi





[[ -n "${THIN_INSTALLER_OVERRIDE}" ]] && export THIN_INSTALLER_OVERRIDE

## Requires sudo permissions:
PRESERVE_ENV=OUTPUT,BASE_DIST,COVFILE,BASE,EXAMPLEPLUGIN_SDDS,THIN_INSTALLER_OVERRIDE,SYSTEM_PRODUCT_TEST_OUTPUT,SSPL_AUDIT_PLUGIN_SDDS,SSPL_PLUGIN_EVENTPROCESSOR_SDDS
LOG_LEVEL=TRACE
EXCLUSION="--exclude manual --exclude WEEKLY"
if [[ -n "${TEST_SELECTOR}" ]]
then
    sudo \
        --preserve-env="${PRESERVE_ENV}" \
        robot --loglevel "${LOG_LEVEL}" ${EXCLUSION} --test "${TEST_SELECTOR}" tests
else
    sudo \
        --preserve-env="${PRESERVE_ENV}" \
        robot --loglevel "${LOG_LEVEL}" ${EXCLUSION} --exclude WEEKLY tests
fi

echo "Tests exited with $?"

scp -i ${PRIVATE_KEY} /tmp/system-tests/log.html upload@allegro.eng.sophos:public_html/bullseye/sspl-systemtest-log.html

exit 0
