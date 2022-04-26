#!/bin/bash
# Copyright Sophos 2021
set -x

ORIGINAL_DIR=$(pwd)

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR

echo ORIGINAL_DIR=$ORIGINAL_DIR
echo SCRIPT_DIR=${SCRIPT_DIR}

ls -l $ORIGINAL_DIR
ls -l $SCRIPT_DIR
ls -l ..
OUTPUT=../av
ls -l .. . $OUTPUT

CREATE_DIR=./gather_dir

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete old $CREATE_DIR"
[[ -d ../test_scripts ]] || failure 22 "Can't find test_scripts at ../test_scripts"
[[ -d $OUTPUT ]] || failure 24 "Failed to find output at $OUTPUT!"


TEST_DIR_NAME=test
TEST_DIR=${CREATE_DIR}/${TEST_DIR_NAME}
INPUTS=${TEST_DIR}/inputs
mkdir -p ${INPUTS}

COPY=mv
# non-destructive
COPY="cp -a"

$COPY ../test_scripts ${INPUTS}/test_scripts || failure 23 "Failed to copy test_scripts"
ln -s test_scripts ${INPUTS}/TA

AV=$INPUTS/av
mkdir -p $AV

rm -rf "$AV/SDDS-COMPONENT"
$COPY "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
chmod 700 "$AV/SDDS-COMPONENT/install.sh"

rm -rf "$AV/base-sdds"
$COPY "$OUTPUT/base-sdds/"      "$AV/base-sdds"
chmod 700 "$AV/base-sdds/install.sh"

rm -rf "$AV/test-resources"
$COPY "$OUTPUT/test-resources"  "$AV/"

# Supplements
rm -rf "$INPUTS/local_rep"
$COPY "../local_rep" "$INPUTS/local_rep"

rm -rf "$INPUTS/vdl"
$COPY "../vdl" "$INPUTS/vdl"

rm -rf "$INPUTS/ml_model"
$COPY "../ml_model" "$INPUTS/ml_model"

echo "Copying test.sh"
cp test.sh ${INPUTS}/test_scripts/test.sh
echo "Copying testAndSendResults.sh"
cp testAndSendResults.sh ${INPUTS}/test_scripts/testAndSendResults.sh

ls -lR ${CREATE_DIR}

# TODO - publish DataSet-A to artifactory removing need for 7zip
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y p7zip-full
PYTHON=${PYTHON:-python3}
${PYTHON} ${INPUTS}/test_scripts/manual/createInstallSet.py "${AV}/INSTALL-SET" "${AV}/SDDS-COMPONENT" "${AV}/.." || failure 2 "Failed to create install-set: $?"

echo "Creating tarfile"
tar czf ${TEST_TAR} -C "$CREATE_DIR"  \
    --exclude='test/inputs/vdl.zip' \
    --exclude='test/inputs/model.zip' \
    --exclude='test/inputs/reputation.zip' \
    . || failure 18 "Failed to create archive of build"

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete $CREATE_DIR"

