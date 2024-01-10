#!/usr/bin/env bash
set -ex

TEST_DIRECTORY="/opt/test/inputs/"
FUZZ_LOG="${TEST_DIRECTORY}log/fuzz-log.txt"
ARTIFACT_OUTPUT="${TEST_DIRECTORY}artifacts/"

#check if any artifacts exist
[[ -d $ARTIFACT_OUTPUT ]] || exit 1
[[ $(find "$ARTIFACT_OUTPUT" -type f | wc -l) == 0 ]] || exit 1

#check if fuzzer ran
[[ -f $FUZZ_LOG ]] || exit 2
LINE=$(grep -E "stat::average_exec_per_sec: " $FUZZ_LOG)
[[ $(echo "$LINE" | grep -E -o  "[0-9]+") != 0 ]] || exit 2