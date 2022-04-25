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

if [[ -n $DATASETA ]]
then
	echo Using DataSetA instead of full Virus Data set
	DATASETA_ARG="--dataseta"
fi

if [[ -f ${DEST_BASE}/output.zip ]]
then
    NEWER="--time-cond ${DEST_BASE}/output.zip"
else
    NEWER=""
fi

curl https://artifactory.sophos-ops.com/${REPO_PATH}/build/output.zip --output ${DEST_BASE}/output.zip $NEWER

OUTPUT=${DEST_BASE}/output
[ -d "$OUTPUT" ] && rm -rf $OUTPUT
mkdir $OUTPUT
unzip ${DEST_BASE}/output.zip -d $OUTPUT

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
${PYTHON} ${BASE}/manual/downloadSupplements.py "$INPUTS" $DATASETA_ARG

ARCHIVE_OPTION=${ARCHIVE_OPTION:-j}

exec tar c${ARCHIVE_OPTION}f /tmp/inputs.tar.bz2 -C ${DEST_BASE} \
    --exclude='test/inputs/vdl.zip' \
    --exclude='test/inputs/model.zip' \
    --exclude='test/inputs/reputation.zip' \
    ${TEST_DIR_NAME}
