#!/bin/bash
set -ex

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

OUTPUT=$1
if [[ -z $OUTPUT ]]
then
    OUTPUT=$BASE/../output
fi

BASE_OUTPUT=$2
if [[ -z $BASE_OUTPUT ]]
then
    BASE_OUTPUT=$OUTPUT/base-sdds
else
    BASE_OUTPUT=$BASE_OUTPUT/SDDS-COMPONENT
fi


DEST_BASE=/tmp
TEST_DIR_NAME=test
TEST_DIR=${DEST_BASE}/${TEST_DIR_NAME}
INPUTS=${TEST_DIR}/inputs
AV=$INPUTS/av
mkdir -p $AV

rsync -va --copy-unsafe-links --delete  "$BASE/../TA/"            "$INPUTS/test_scripts"
ln -snf test_scripts "$INPUTS/TA"
rsync -va --copy-unsafe-links --delete "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
rsync -va --copy-unsafe-links --delete "${BASE_OUTPUT}/"         "$AV/base-sdds"
rsync -va --copy-unsafe-links --delete "$OUTPUT/test-resources"  "$AV/"
exec tar cjf /tmp/inputs.tar.bz2 -C ${DEST_BASE} ${TEST_DIR_NAME}

## To unpack:
# copy /tmp/inputs.tar.bz2 to test machine
# cd /opt
# tar xjf <tarfile>
