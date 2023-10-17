#!/bin/bash

set -x
##

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

TEST_TAR=$1
[[ -z "$TEST_TAR" ]] && failure 31 "Pass a path to the tarfile to create as the first argument"
CREATE_DIR=./testUtils

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete old $CREATE_DIR"

# Assume (for jenkins job) that base is already present in the directory above aws-runner
cp -r ../base/testUtils ${CREATE_DIR}
cp -r ../common/TA/libs ${CREATE_DIR}
cp -r ../common/TA/robot ${CREATE_DIR}


export TEST_UTILS=${CREATE_DIR}
export SYSTEMPRODUCT_TEST_INPUT=./system-product-test-inputs
# this changes the working directory of the gather process to a relative path to the job so that we can run
# multiple jobs on the same machine
sed -i s:/tmp/system-product-test-inputs:${SYSTEMPRODUCT_TEST_INPUT}:g ${TEST_UTILS}/system-product-test-release-package.xml
source ${TEST_UTILS}/SupportFiles/jenkins/gatherTestInputs.sh || failure 211 "Failed to gather inputs"
source ${TEST_UTILS}/SupportFiles/jenkins/exportInputLocations.sh
source ${TEST_UTILS}/SupportFiles/jenkins/checkTestInputsAreAvailable.sh || failure 211 "Failed to gather inputs"
${TEST_UTILS}/SupportFiles/jenkins/SetupCIBuildScripts.sh --download-pip-cache ${CREATE_DIR} || failure 211 "Failed to gather inputs"
python3 -m pip install boto
python3 -m pip install boto3 pytz

if [[ -d /home/jenkins ]]
then
  python3.8 ${TEST_UTILS}/libs/DownloadAVSupplements.py  ${SYSTEMPRODUCT_TEST_INPUT}
else
  python3 ${TEST_UTILS}/libs/DownloadAVSupplements.py  ${SYSTEMPRODUCT_TEST_INPUT}
fi
([[ -d ${SYSTEMPRODUCT_TEST_INPUT} ]] && tar czf ${CREATE_DIR}/SystemProductTestInputs.tgz ${SYSTEMPRODUCT_TEST_INPUT}) || failure 212 "Failed to tar inputs"
rm -rf "${SYSTEMPRODUCT_TEST_INPUT}" || failure 21 "Failed to delete new ${SYSTEMPRODUCT_TEST_INPUT}"

echo "Copying test.sh"
cp test.sh $CREATE_DIR/test.sh
echo "Copying testAndSendResults.sh"
cp testAndSendResults.sh $CREATE_DIR/testAndSendResults.sh

echo "Creating tarfile"
tar czf ${TEST_TAR} -C "$CREATE_DIR" . || failure 18 "Failed to create archive of build"

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete new $CREATE_DIR"


