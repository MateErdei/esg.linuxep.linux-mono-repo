#!/bin/bash
set -ex

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

REPO_PATH=$1
if [[ -z $REPO_PATH ]]
then
	echo Please pass an artifactory repository path
	exit 1
fi

curl https://artifactory.sophos-ops.com/${REPO_PATH}/build/output.zip --output output.zip

DEST_BASE=/tmp
OUTPUT=${DEST_BASE}/output
mkdir $OUTPUT

unzip output.zip -d $OUTPUT

TEST_DIR_NAME=test
TEST_DIR=${DEST_BASE}/${TEST_DIR_NAME}
INPUTS=${TEST_DIR}/inputs
AV=$INPUTS/av
mkdir -p $AV

rsync -va --copy-unsafe-links --delete  "$BASE/../TA/"            "$INPUTS/test_scripts"
ln -snf test_scripts "$INPUTS/TA"
rsync -va --copy-unsafe-links --delete "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
rsync -va --copy-unsafe-links --delete "$OUTPUT/base-sdds/"      "$AV/base-sdds"
rsync -va --copy-unsafe-links --delete "$OUTPUT/test-resources"  "$AV/"

./manual/createInstallSet.py "$OUTPUT/INSTALL-SET/" "$OUTPUT/SDDS-COMPONENT/" "$OUTPUT/base-sdds/"

exec tar cjf /tmp/inputs.tar.bz2 -C ${DEST_BASE} ${TEST_DIR_NAME}
