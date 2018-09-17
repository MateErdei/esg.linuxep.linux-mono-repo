#!/usr/bin/env bash

## Can't always delete the COVFILE,
## since we would need to do a clean build if we delete the covfile
## Because the COVFILE records the 'zero's for all compiled code.
## So we don't get any coverage if a file isn't compiled after the COVFILE
## is created
#~ rm -f $COVFILE

CLEAN=0
if [[ ! -f $COVFILE ]]
then
    mkdir -p $(dirname $COVFILE)
    covmgr -l -c
    cov01 -1
    CLEAN=1
fi

covselect --deleteAll

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
currentDir="../..$BASE_DIR"

SCRIPT_DIR=$(cd "${0%/*}"; echo "$PWD")
SRC_DIR="${SCRIPT_DIR%/*}"

echo "COVFILE=$COVFILE"
echo "COVDIR=$COVDIR"
echo "ORIG_CWD=$ORIG_CWD"
echo "BASE_DIR=$BASE_DIR"
echo "SCRIPT_DIR=$SCRIPT_DIR"
echo "SRC_DIR=$SRC_DIR"
echo "rel path = $currentDir"

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

echo "Excluding \!../../redist/"
covselect --quiet --add \!../../redist/
echo "Excluding \!../../opt/"
covselect --quiet --add \!../../opt/
echo "Excluding \!../../lib/"
covselect --quiet --add \!../../lib/

covselect --list --no-banner

