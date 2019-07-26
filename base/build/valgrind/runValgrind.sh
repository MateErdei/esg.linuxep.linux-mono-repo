#!/usr/bin/env bash


#valgrind --leak-check=full --error-exitcode=70 \
#    --trace-children=yes \
#    --show-leak-kinds=definite,possible,indirect \
#    --suppressions=$BASE/build/valgrind/suppressions.supp \
#    --gen-suppressions=all \
#    ctest --parallel ${NPROC} --output-on-failure || {
#    local EXITCODE=$?
#    exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"

[[ -n ${BASE} ]] || BASE=$(pwd)
BUILD_DIR=$BASE/build64
export MEMORYCHECK_SUPPRESSIONS_FILE=$BASE/build/valgrind/suppressions.supp
export MEMORYCHECK_COMMAND_OPTIONS="--gen-suppressions=all"
[[ -n ${NPROC:-} ]] || NPROC=2

cd ${BUILD_DIR}
rm -rf Testing/Temporary/*

#ctest -VV --debug --test-action memcheck
pwd
echo 'Suppressions'
cat ${MEMORYCHECK_SUPPRESSIONS_FILE}
echo 'Run Ctest'
ctest \
    -D MEMORYCHECK_SUPPRESSIONS_FILE=${MEMORYCHECK_SUPPRESSIONS_FILE} \
    -D CTEST_MEMORYCHECK_SUPPRESSIONS_FILE=${MEMORYCHECK_SUPPRESSIONS_FILE} \
    -D MEMORYCHECK_COMMAND_OPTIONS="${MEMORYCHECK_COMMAND_OPTIONS}" \
    -D CTEST_MEMORYCHECK_COMMAND_OPTIONS="${MEMORYCHECK_COMMAND_OPTIONS}" \
    --test-action memcheck --parallel ${NPROC} \
    --output-on-failure \
    -E 'ReactorCallTerminatesIfThePollerBreaksForZMQSockets|ReactorCallTerminatesIfThePollerBreaks|PollerShouldThrowExceptionIfUnderlingSocketCloses|PythonTest'

pushd Testing/Temporary

# remove all the tests that passed
grep  'ERROR SUMMARY: 0' MemoryChecker*.log | cut -d':' -f1 | xargs rm

# consider a success if no memory error is reported by valgrind
EXIT=0
Failures=`ls MemoryChecker*.log 2>/dev/null | wc -l`
if [[ Failures -gt  0 ]]; then
    EXIT=1
    for file in MemoryChecker*.log; do
        cat ${file};
    done
fi
popd

[[ ${EXIT} == 0 ]] || echo "Memory Checker failed: $EXIT"
exit ${EXIT}

