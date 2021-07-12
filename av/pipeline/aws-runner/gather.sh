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

CREATE_DIR=./gather_dir

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete old $CREATE_DIR"

[[ -d ../TA ]] || failure 22 "Can't find TA at ../TA"
cp -r ../TA ${CREATE_DIR} || failure 23 "Failed to copy TA"

BASE=${CREATE_DIR}
export TEST_UTILS=${CREATE_DIR}

# Not sure where pipeline will put this
OUTPUT=../output
ls -l .. . $OUTPUT
[[ -d $OUTPUT ]] || failure 24 "Failed to find output at $OUTPUT!"

TEST_DIR_NAME=test
TEST_DIR=${DEST_BASE}/${TEST_DIR_NAME}
INPUTS=${TEST_DIR}/inputs
AV=$INPUTS/av
mkdir -p $AV

rsync -va --copy-unsafe-links --delete  "$BASE/../TA/"            "$INPUTS/test_scripts"
ln -snf test_scripts "$INPUTS/TA"
rm -rf "$AV/SDDS-COMPONENT"
mv "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
chmod 700 "$AV/SDDS-COMPONENT/install.sh"
rm -rf "$AV/base-sdds"
mv "$OUTPUT/base-sdds/"      "$AV/base-sdds"
chmod 700 "$AV/base-sdds/install.sh"
rm -rf "$AV/test-resources"
mv "$OUTPUT/test-resources"  "$AV/"

PYTHON=${PYTHON:-python3}
${PYTHON} ${BASE}/manual/downloadSupplements.py "$INPUTS"

echo "Copying test.sh"
cp test.sh $CREATE_DIR/test.sh
echo "Copying testAndSendResults.sh"
cp testAndSendResults.sh $CREATE_DIR/testAndSendResults.sh

ls -lR ${CREATE_DIR}

echo "Creating tarfile"
tar czf ${TEST_TAR} -C "$CREATE_DIR"  \
    --exclude='test/inputs/vdl.zip' \
    --exclude='test/inputs/model.zip' \
    --exclude='test/inputs/reputation.zip' \
    . || failure 18 "Failed to create archive of build"

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete $CREATE_DIR"

