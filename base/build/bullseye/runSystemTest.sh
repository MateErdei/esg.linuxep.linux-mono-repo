#!/usr/bin/env bash

## Run the system tests with the output from a build

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")

[[ -n ${BASE} ]] || BASE=${SCRIPT_DIR}/../..
export BASE
[[ -n ${OUTPUT} ]] || OUTPUT=$BASE/output
export OUTPUT

if [[ -n "$COVFILE" ]]
then
    echo "Creating links for COVFILE $COVFILE"
    sudo ln -sf "$COVFILE" /root/fulltest.cov
    sudo ln -sf "$COVFILE" regressionSuite/test.cov
    sudo ln -sf "$COVFILE" /test.cov
    COVDIR=$(dirname $COVFILE)
    echo "COVFILE=$COVFILE" >/tmp/BullseyeCoverageEnv.txt
    echo "COVDIR=$COVDIR" >>/tmp/BullseyeCoverageEnv.txt
    sudo chmod 0644 /tmp/BullseyeCoverageEnv.txt
    sudo chmod 0666 $COVFILE
    sudo chmod a+x $COVDIR
else
    echo "No COVFILE specified"
    exit 78
fi
export COVFILE

PRIVATE_KEY=${SCRIPT_DIR}/private.key
chmod 600 ${PRIVATE_KEY}
export GIT_SSH_COMMAND="ssh -i ${PRIVATE_KEY}"

SYSTEM_TEST_CHECKOUT=/tmp/system-tests

if [[ -d ${SYSTEM_TEST_CHECKOUT}/.git ]]
then
    cd ${SYSTEM_TEST_CHECKOUT}
    git pull
else
    git clone --depth 1 ssh://git@stash.sophos.net:7999/linuxep/everest-systemproducttests.git ${SYSTEM_TEST_CHECKOUT}
    cd ${SYSTEM_TEST_CHECKOUT}
fi

[[ -f ./tests/__init__.robot ]] || {
    echo "Failed to checkout system tests"
    exit 77
}

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

## Requires sudo permissions:
exec sudo --preserve-env=OUTPUT,BASE_DIST,COVFILE,BASE,EXAMPLE_PLUGIN_SDDS \
    robot --loglevel TRACE --exclude manual tests
