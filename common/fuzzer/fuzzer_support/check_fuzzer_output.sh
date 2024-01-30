#!/usr/bin/env bash
set -ex

TEST_DIRECTORY="/opt/test/inputs/"
FUZZ_LOG="${TEST_DIRECTORY}log/fuzz-log.txt"
ARTIFACT_OUTPUT="${TEST_DIRECTORY}artifacts/"

#check if any artifacts exist
[[ -d $ARTIFACT_OUTPUT ]] || exit 1
[[ $(find "$ARTIFACT_OUTPUT" -type f | wc -l) == 0 ]] || { (ls "$ARTIFACT_OUTPUT"); exit 1; }


#check if fuzzer created a log
[[ -f $FUZZ_LOG ]] || { echo "no fuzz log"; exit 2; }

#check if sanitizers found anything
[[ $(grep -c Sanitizer $FUZZ_LOG) == 0 ]] || { grep Sanitizer $FUZZ_LOG; exit 3; }

#check if fuzzer completed
LINE=$(grep -E "stat::average_exec_per_sec: " $FUZZ_LOG)
[[ -n $LINE ]] || { echo "fuzzer exited early"; exit 4;  }

#check fuzzer executed test runs
[[ $(echo "$LINE" | grep -E -o  "[0-9]+") != 0 ]] || { echo "average_exec_per_sec is 0"; exit 5; }
echo "$LINE"