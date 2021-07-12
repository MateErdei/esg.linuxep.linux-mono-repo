#!/bin/bash

set -x

# Set this variable to true if you want reruns on failed tests
RERUNFAILED=${RERUNFAILED:-false}
STACKNAME=$1
shift

TEST_SCRIPTS=/opt/test/inputs/test_scripts

$TEST_SCRIPTS/test.sh "$@"
RESULT=$?

## Upload results to s3
UPLOAD_RESULT=0
aws s3 cp $TEST_SCRIPTS/log.html s3://sspl-testbucket/test-results/$STACKNAME/$HOSTNAME-log.html && \
    aws s3 cp $TEST_SCRIPTS/output.xml s3://sspl-testbucket/test-results/$STACKNAME/$HOSTNAME-output.xml && \
    aws s3 cp $TEST_SCRIPTS/report.html s3://sspl-testbucket/test-results/$STACKNAME/$HOSTNAME-report.html && \
    aws s3 cp $TEST_SCRIPTS/robot.xml s3://sspl-testbucket/test-results/$STACKNAME/$HOSTNAME-robot.xml && \
    aws s3 cp /tmp/cloudFormationInit.log s3://sspl-testbucket/test-results/$STACKNAME/$HOSTNAME-cloudFormationInit.log || UPLOAD_RESULT=0


[[ $RESULT == 0 ]] || sleep 100
