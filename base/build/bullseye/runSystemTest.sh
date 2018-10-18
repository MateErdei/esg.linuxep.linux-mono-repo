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
    git pull \
            || failure 80 "Failed to pull system tests"
    git checkout "${BULLSEYE_SYSTEM_TEST_BRANCH}" \
            || failure 81 "Failed to checkout required branch"
else
    git clone \
            --branch "${BULLSEYE_SYSTEM_TEST_BRANCH}" \
            --single-branch \
            --depth 1 \
            ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git "${SYSTEM_TEST_CHECKOUT}" \
            || failure 82 "Failed to clone system tests"
    cd "${SYSTEM_TEST_CHECKOUT}"
fi

[[ -f ./tests/__init__.robot ]] || failure 77 "Failed to checkout system tests"

ln -nsf "$COVFILE" test.cov
ln -nsf "$COVFILE" .

## Find example plugin
if [[ -d "$EXAMPLE_PLUGIN_SDDS" ]]
then
    export EXAMPLE_PLUGIN_SDDS
else
    FILER_6_LINUX=NOT_FOUND
    [[ -d /mnt/filer6/linux/SSPL ]] && FILER_6_LINUX=/mnt/filer6/linux
    [[ -d /uk-filer6/linux/SSPL ]] && FILER_6_LINUX=/uk-filer6/linux

    FILER_5_BIR=NOT_FOUND
    [[ -d /uk-filer5/prodro/bir/sspl-exampleplugin ]] && FILER_5_BIR=/uk-filer5/prodro/bir
    [[ -d /mnt/filer5/prodro/bir/sspl-exampleplugin ]] && FILER_5_BIR=/mnt/filer5/prodro/bir

    if [[ -d "$FILER_6_LINUX/SSPL/JenkinsBuildOutput/ExamplePlugin/master/SDDS-COMPONENT" ]]
    then
        export EXAMPLE_PLUGIN_SDDS=$FILER_6_LINUX/SSPL/JenkinsBuildOutput/ExamplePlugin/master/SDDS-COMPONENT
    elif [[ -d "$FILER_5_BIR/sspl-exampleplugin" ]]
    then
        DIR=$(ls -1 "$FILER_5_BIR/sspl-exampleplugin/0-*/*/output/SDDS-COMPONENT" | sort -rV | head -1)
        if [[ -d "$DIR" ]]
        then
            export EXAMPLE_PLUGIN_SDDS="$DIR"
        fi
    fi
fi

[[ -n "${THIN_INSTALLER_OVERRIDE}" ]] && export THIN_INSTALLER_OVERRIDE

## Requires sudo permissions:
sudo \
    --preserve-env=OUTPUT,BASE_DIST,COVFILE,BASE,EXAMPLE_PLUGIN_SDDS,THIN_INSTALLER_OVERRIDE \
    robot --loglevel TRACE --exclude manual tests

scp -i ${PRIVATE_KEY} /tmp/system-tests/log.html upload@allegro.eng.sophos:public_html/bullseye/sspl-systemtest-log.html

exit 0
