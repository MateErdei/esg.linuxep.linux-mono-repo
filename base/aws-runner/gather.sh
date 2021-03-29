#!/bin/bash

set -x
##

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR



CREATE_DIR=/tmp/testUtils

function failure()
{
    local CODE=$1
    shift
    echo "$@"
    exit $CODE
}

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete old $CREATE_DIR"

# Assume (for jenkins job) that Everest-Base is already present in the directory above SSPL-AWS-Runner
cp -r ../testUtils ${CREATE_DIR}
echo $@ > ${CREATE_DIR}/robotArgs



sudo -H ${CREATE_DIR}/SupportFiles/jenkins/SetupCIBuildScripts.sh || failure 210 "Failed to install CI scripts"
export TEST_UTILS=${CREATE_DIR}
pushd ${CREATE_DIR}
source ${CREATE_DIR}/SupportFiles/jenkins/gatherTestInputs.sh || failure 211 "Failed to gather inputs"
source ${CREATE_DIR}/SupportFiles/jenkins/exportInputLocations.sh
source ${CREATE_DIR}/SupportFiles/jenkins/checkTestInputsAreAvailable.sh || failure 211 "Failed to gather inputs"
popd

([[ -d /tmp/system-product-test-inputs ]] && tar czf ${CREATE_DIR}/SystemProductTestInputs.tgz -C /tmp system-product-test-inputs/) || failure 212 "Failed to tar inputs"

echo "Copying test.sh"
cp test.sh $CREATE_DIR/test.sh
echo "Copying testAndSendResults.sh"
cp testAndSendResults.sh $CREATE_DIR/testAndSendResults.sh

echo "Creating tarfile"
tar czf /tmp/sspl-test.tgz -C "$CREATE_DIR" . || failure 18 "Failed to create archive of build"

rm -rf "${CREATE_DIR}" || failure 20 "Failed to delete new $CREATE_DIR"


