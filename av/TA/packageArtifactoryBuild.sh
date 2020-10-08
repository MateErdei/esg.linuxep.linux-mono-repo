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

DEST_BASE=/tmp
OUTPUT=${DEST_BASE}/output
[ -d "$OUTPUT" ] && rm -rf $OUTPUT
mkdir $OUTPUT

curl https://artifactory.sophos-ops.com/${REPO_PATH}/build/output.zip --output ${DEST_BASE}/output.zip
unzip ${DEST_BASE}/output.zip -d $OUTPUT

TEST_DIR_NAME=test
TEST_DIR=${DEST_BASE}/${TEST_DIR_NAME}
INPUTS=${TEST_DIR}/inputs
AV=$INPUTS/av
mkdir -p $AV

mv "$BASE/../TA/"            "$INPUTS/test_scripts"
ln -snf test_scripts "$INPUTS/TA"
mv "$OUTPUT/SDDS-COMPONENT/" "$AV/SDDS-COMPONENT"
chmod 700 "$AV/SDDS-COMPONENT/install.sh"
mv "$OUTPUT/base-sdds/"      "$AV/base-sdds"
chmod 700 "$AV/base-sdds/install.sh"
mv "$OUTPUT/test-resources"  "$AV/"

PYTHON=${PYTHON:-python3}
${PYTHON} ${BASE}/manual/downloadSupplements.py "$INPUTS"

exec tar cjf /tmp/inputs.tar.bz2 -C ${DEST_BASE} ${TEST_DIR_NAME}
