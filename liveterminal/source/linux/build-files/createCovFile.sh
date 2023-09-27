#!/usr/bin/env bash

## Can't always delete the COVFILE,
## since we would need to do a clean build if we delete the covfile
## Because the COVFILE records the 'zero's for all compiled code.
## So we don't get any coverage if a file isn't compiled after the COVFILE
## is created
#~ rm -f $COVFILE

function failure()
{
    local E=$1
    shift
    echo "$@"
    exit $E
}

[[ -n ${COVFILE} ]] || failure 2 "COVFILE not set!"

if [[ -f /pandorum/BullseyeLM/BullseyeCoverageLicenseManager ]]
then
    covlmgr -f /pandorum/BullseyeLM/BullseyeCoverageLicenseManager --use || failure 3 "Unable to use licence /pandorum/BullseyeLM/BullseyeCoverageLicenseManager"
elif [[ -f /root/BullseyeCoverageLicenseManager ]]
then
    covlmgr -f /root/BullseyeCoverageLicenseManager --use \
        || failure 3 "Unable to use licence /root/BullseyeCoverageLicenseManager"
fi

CLEAN=0
if [[ ! -f ${COVFILE} ]]
then
    mkdir -p $(dirname ${COVFILE})
    chmod 777 $(dirname ${COVFILE})
    covmgr -l -c
    cov01 -1

    [[ -f ${COVFILE} ]] || {
        echo "Failed to create COVFILE: $?"
        exit 1
    }
    CLEAN=1
fi

chmod 666 "${COVFILE}"
covselect --deleteAll || failure 10 "Failed to clean cov selections"

## cov exclusions need to be relative to the COVFILE

## We start off in the src directory
## e.g. source-checkout
ORIG_CWD="$PWD"

BASE_DIR=$1

if [[ $CLEAN == 1 ]]
then
    echo "Doing a clean build"
    rm -rf $BASE_DIR/build64
fi

## COVFILE is in /tmp/root
COVDIR="${COVFILE%/*}"

[[ $COVDIR == "/tmp/root" ]] || echo "UNEXPECTED COVDIR = $COVDIR"

SCRIPT_DIR="${BASE_DIR}/source/linux/build-files"
SRC_DIR="${BASE_DIR}"

echo "COVFILE=$COVFILE"
echo "COVDIR=$COVDIR"
echo "BASE_DIR=$BASE_DIR"
echo "SCRIPT_DIR=$SCRIPT_DIR"
echo "SRC_DIR=$SRC_DIR"

function exclude()
{
    echo "covselect --add $*"
    covselect --add $@ || failure 3 "Failed to add exclusion $*"
}

echo "Excluding \!../../build/redist/"
covselect --quiet --add \!../../build/redist/ || failure 4 "Failed to exclude /build/redist"
echo "Excluding \!../../opt/"
covselect --quiet --add \!../../opt/ || failure 5 "Failed to exclude /opt"
echo "Excluding \!../../lib/"
covselect --quiet --add \!../../lib/ || failure 6 "Failed to exclude /lib"

SRC_TEST_DIR=${SRC_DIR}/source/linux/tests

[[ -d ${SRC_TEST_DIR} ]] || {
    echo "Failed to find src dir - SRC_TEST_DIR doesn't exist"
    exit 2
}

exclude \!../..${SRC_DIR}/build/
exclude \!../..${SRC_DIR}/build64/
exclude \!../..${SRC_DIR}/source/_input/
exclude \!../..${SRC_TEST_DIR}/
exclude \!../..${SRC_DIR}/log/
exclude \!../..${SRC_DIR}/ta/
exclude \!../..${SRC_DIR}/tests/

echo "Exclusions:"
covselect --list --no-banner
