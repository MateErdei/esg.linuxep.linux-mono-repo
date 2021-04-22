#!/bin/bash

JOB_ID=$1
shift
function failure()
{
    echo "$@"
    exit 1
}


SCRIPT_DIR="${0%/*}"
[[ $SCRIPT_DIR == $0 ]] && SCRIPT_DIR=.
cd $SCRIPT_DIR


export TEST_TAR=./sspl-test-${JOB_ID}.tgz
TAR_BASENAME=$(basename ${TEST_TAR})
## Gather files
if [[ -z "$SKIP_GATHER" ]]
then
    bash ./gather.sh || failure "Failed to gather test files: $?"
fi
[[ -f "$TEST_TAR" ]] || failure "Failed to gather test files: $TEST_TAR doesn't exist"

## Create template


TAR_DESTINATION_FOLDER="s3://sspl-testbucket/sspl"
## Upload test tarfile to s3
if [[ -z "$SKIP_TAR_COPY" ]]
then
    aws s3 cp "$TEST_TAR" ${TAR_DESTINATION_FOLDER}/${TAR_BASENAME} || failure "Unable to copy test tarfile to s3"
fi
rm $TEST_TAR