#!/bin/bash
set -ex

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

REPO_PATH=$1
if [[ -z $REPO_PATH ]]
then
	echo Please pass an artifactory repository path like esg-build-tested/linuxep.sspl-plugin-anti-virus/develop/20220920115213-a74084511be8327fba69897c92852d2e1eb37a35-vYMroO/

	exit 1
fi
DEST_BASE=/tmp/av
mkdir -p $DEST_BASE

if [[ -f ${DEST_BASE}/output.zip ]]
then
    NEWER="--time-cond ${DEST_BASE}/output.zip"
else
    NEWER=""
fi

curl https://artifactory.sophos-ops.com/artifactory/${REPO_PATH}/build/output.zip --output ${DEST_BASE}/output.zip $NEWER

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

if [[ -n $SYMBOLS ]]
then
    rm -rf "$INPUTS/symbols"
    mv "$OUTPUT/symbols"  "$INPUTS/symbols"
fi

PYTHON=${PYTHON:-python3}
${PYTHON} ${BASE}/manual/downloadSupplements.py "$INPUTS"

if [[ -n $TEST_TAR ]]
then
    echo "Packaging for automatic test runs to $TEST_TAR"
    DEST=$TEST_TAR
    cp $BASE/../pipeline/aws-runner/test.sh "$INPUTS/test_scripts/"
    cp $BASE/../pipeline/aws-runner/testAndSendResults.sh "$INPUTS/test_scripts/"
else
    DEST=/tmp/ssplav-inputs.tar.bz2
    echo "Packaging for manual test run to $DEST"
fi

case $DEST in
    *.tar.gz|*.tgz)
        DEFAULT_ARCHIVE_OPTION=z
        ;;
    *.tar.bz2)
        DEFAULT_ARCHIVE_OPTION=j
        ;;
    *.tar)
        DEFAULT_ARCHIVE_OPTION=
        ;;
    *)
        echo "Unknown archive type: $DEST"
        ;;
esac

ARCHIVE_OPTION=${ARCHIVE_OPTION:-${DEFAULT_ARCHIVE_OPTION}}

exec tar c${ARCHIVE_OPTION}f ${DEST} -C ${DEST_BASE} \
    --exclude='test/inputs/dataseta.zip' \
    --exclude='test/inputs/model.zip' \
    --exclude='test/inputs/reputation.zip' \
    ${TEST_DIR_NAME}
