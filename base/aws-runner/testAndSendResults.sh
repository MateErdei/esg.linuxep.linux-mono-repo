#!/bin/bash

set -x

# Set this variable to true if you want reruns on failed tests
RERUNFAILED=${RERUNFAILED:-false}
STACKNAME=$1
shift

/opt/sspl/test.sh "$@"
RESULT=$?

## Upload results to s3
UPLOAD_RESULT=0
aws s3 cp /opt/sspl/log.html s3://sspl-testbucket/test-results/$1/$HOSTNAME-log.html && \
    aws s3 cp /opt/sspl/output.xml s3://sspl-testbucket/test-results/$1/$HOSTNAME-output.xml && \
    aws s3 cp /opt/sspl/report.html s3://sspl-testbucket/test-results/$1/$HOSTNAME-report.html && \
    aws s3 cp /opt/sspl/robot.xml s3://sspl-testbucket/test-results/$1/$HOSTNAME-robot.xml && \
    aws s3 cp /tmp/cloudFormationInit.log s3://sspl-testbucket/test-results/$1/$HOSTNAME-cloudFormationInit.log || UPLOAD_RESULT=0


[[ $RESULT == 0 ]] || sleep 100
