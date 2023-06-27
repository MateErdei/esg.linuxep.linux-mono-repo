#!/bin/bash

function fail()
{
    echo "----------"
    echo $1
    echo "----------"
    exit 1
}

if [[ -z $TEST ]]
then
    fail "Usage: 'TEST=<test> make kcov', e.g. TEST=watchdog/watchdogimpl/TestWatchdog make kcov"
    exit 1
fi

INCLUDE_DIR=$1
EXCLUDE_DIR=$2
OUTPUT_DIR=$3
TEST_DIR=$4

echo "TEST = $TEST"

KCOV_COMMAND="kcov --include-path ${INCLUDE_DIR} --exclude-path ${EXCLUDE_DIR} ${OUTPUT_DIR} ${TEST_DIR}/$TEST"

echo "Running:  $KCOV_COMMAND"

KCOV_PATH=$(which kcov)
[[ -z $KCOV_PATH ]] && {
    fail "Please install kcov and ensure it is on the PATH"
}

$KCOV_COMMAND || {
    fail "Failed to run kcov"
}

echo "Your output HTML files are located here: ${OUTPUT_DIR}"
echo "To View: firefox ${OUTPUT_DIR}/index.html"

