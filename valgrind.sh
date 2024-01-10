#!/bin/bash

# Syntax: ./valgrind.sh <bazel test targets / exclusions>
# e.g. VALGRIND=/home/douglas/valgrind/bin/valgrind ./valgrind.sh //av/tests/avscanner/avscannerimpl:TestAVScannerImpl

SUPPRESSIONS_DIR=$(pwd)/common/valgrind
VALGRIND=${VALGRIND:-valgrind}

echo Using $VALGRIND

exec bazel test --tool_tag=ijwb:CLion --curses=yes --color=yes --config=linux_x64_dbg --test_output=all \
  --runs_per_test=1 --flaky_test_attempts=1 \
  --run_under="$VALGRIND --error-exitcode=70 --gen-suppressions=all --suppressions=${SUPPRESSIONS_DIR}/unittest.suppressions" -- \
  "$@"
