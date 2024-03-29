#!/usr/bin/env bash


#valgrind --leak-check=full --error-exitcode=70 \
#    --trace-children=yes \
#    --show-leak-kinds=definite,possible,indirect \
#    --suppressions=$BASE/build/valgrind/suppressions.supp \
#    --gen-suppressions=all \
#    ctest --parallel ${NPROC} --output-on-failure || {
#    local EXITCODE=$?
#    exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"

BASE=$(pwd)
BUILD_DIR=$BASE/build64
export MEMORYCHECK_SUPPRESSIONS_FILE=$BASE/build/valgrind/suppressions.supp
export MEMORYCHECK_COMMAND_OPTIONS="--gen-suppressions=all"
NPROC=8

cd ${BUILD_DIR}

ctest -VV --debug \
    -D MEMORYCHECK_SUPPRESSIONS_FILE=${MEMORYCHECK_SUPPRESSIONS_FILE} \
    -D CTEST_MEMORYCHECK_SUPPRESSIONS_FILE=${MEMORYCHECK_SUPPRESSIONS_FILE} \
    -D MEMORYCHECK_COMMAND_OPTIONS="${MEMORYCHECK_COMMAND_OPTIONS}" \
    -D CTEST_MEMORYCHECK_COMMAND_OPTIONS="${MEMORYCHECK_COMMAND_OPTIONS}" \
    --test-action memcheck --parallel ${NPROC} \
    --output-on-failure \
    || echo "ctest failed: $?"

#ctest -VV --debug --test-action memcheck

