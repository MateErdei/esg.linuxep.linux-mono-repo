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
    mkdir -p "$(dirname ${COVFILE})"
    chmod 777 "$(dirname ${COVFILE})"
    covmgr --list --create
    cov01 --on

    if [[ ! -f ${COVFILE} ]]
    then
        echo "Failed to create COVFILE: $?"
        exit 1
    fi
    CLEAN=1
fi

chmod 666 "${COVFILE}"
covselect --deleteAll || failure 10 "Failed to clean cov selections"

## cov exclusions need to be relative to the COVFILE

## We start off in the src directory
## e.g. source-checkout
ORIG_CWD="$PWD"

## Strip off the last dir
## e.g. source-checkout
BASE_DIR="${ORIG_CWD}"

if [[ $CLEAN == 1 ]]
then
    echo "Doing a clean build"
    rm -rf $BASE_DIR/build64
fi

## COVFILE is in /tmp/root
COVDIR="${COVFILE%/*}"

[[ $COVDIR == "/tmp/root" ]] || echo "UNEXPECTED COVDIR = $COVDIR"

## Need to get between these dirs
#currentDir="../..$BASE_DIR"

#SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")
#BUILD_SRC_DIR="${SCRIPT_DIR%/*}"
#SRC_DIR="${BUILD_SRC_DIR%/*}"

echo "COVFILE=$COVFILE"
echo "COVDIR=$COVDIR"
echo "ORIG_CWD=$ORIG_CWD"
echo "BASE_DIR=$BASE_DIR"
#echo "SCRIPT_DIR=$SCRIPT_DIR"
#echo "SRC_DIR=$SRC_DIR"
#echo "rel path = $currentDir"

#
#EXCLUSION_FILE=$COVDIR/$currentDir/src/regression/supportFiles/bullseye/bullseyeExclusionFile.txt
#
#[ -f $EXCLUSION_FILE ] || {
#    echo "Relative path between $COVFILE and $BASE_DIR is incorrect!"
#    echo "Path tried = $EXCLUSION_FILE"
#    [[ -f ${SCRIPT_DIR}/bullseyeExclusionFile.txt ]] || echo "Can't find exclusion file at all!"
#    exit 1
#}
#
#IFS="$(printf '\n\t')"
#for f in $(cat $EXCLUSION_FILE)
#do
#    echo "Excluding  \!${currentDir}\/$f"
#    covselect --quiet --add \!${currentDir}\/$f
#done

function exclude()
{
    echo "covselect --add $*"
    covselect --add "$@" || failure 3 "Failed to add exclusion $*"
}

exclude \!build/
exclude \!build64/
exclude \!input/
exclude \!redist/
exclude \!tests/
exclude \!tools/

echo "Exclusions:"
covselect --list --no-banner
