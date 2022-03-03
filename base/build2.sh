#!/bin/bash

echo "Running with env:"
env
echo "------"

PRODUCT=sspl-base
export PRODUCT_NAME="Sophos Server Protection Linux - Base Component"
export PRODUCT_LINE_ID="ServerProtectionLinux-Base-component"
export DEFAULT_HOME_FOLDER="sspl-base"

FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
FAILURE_COPY_CPPCHECK_RESULT_FAILED=61
FAILURE_CPPCHECK=62

set -ex
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output
export BASE
export OUTPUT


function source_file()
{
  local path=$1
  [ -f "$path" ] || { echo "Can't find $path" ; exit 11 ; }
  source "$path"
}

source_file "$BASE/build/common.sh"
source_file "$BASE/setup_env_vars.sh"

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

PythonCoverage="OFF"
STRACE_SUPPORT="OFF"
CLEAN=0
#BULLSEYE=0
#BULLSEYE_UPLOAD=0
#BULLSEYE_SYSTEM_TESTS=0
#export NO_REMOVE_GCC=1
#INPUT=/build/input

#COVFILE="/tmp/root/sspl-base-unittest.cov"
#BULLSEYE_SYSTEM_TEST_BRANCH=develop
#export TEST_SELECTOR=

#CMAKE_BUILD_TYPE=RelWithDebInfo
CMAKE_BUILD_TYPE=Debug

#DEBUG=0
export ENABLE_STRIP=1
#VALGRIND=0
UNIT_TESTS=1


# Deal with arguments

while [[ $# -ge 1 ]]
do
    case $1 in
        --clean-log)
            rm -f $LOG
            ;;
        --clean)
            CLEAN=1
            ;;
        --no-clean|--noclean)
            CLEAN=0
            ;;
        --remove-gcc)
            NO_REMOVE_GCC=0
            ;;
        --input)
            shift
            INPUT=$1
            ;;
        --debug)
            CMAKE_BUILD_TYPE=Debug
            DEBUG=1
            export ENABLE_STRIP=0
            ;;
        --999)
            export VERSION_OVERRIDE=99.9.9.999
            ;;
        --060)
            export VERSION_OVERRIDE=0.6.0.999
            export O_SIX_O=1
            touch also_a_fake_lib.so.5.86.999
            touch fake_lib.so.1.66.999
            touch faker_lib.so.2.23.999
            ;;
        --release|--no-debug)
            CMAKE_BUILD_TYPE=RelWithDebInfo
            DEBUG=0
            export ENABLE_STRIP=1
            ;;
        --build-type)
            shift
            CMAKE_BUILD_TYPE="$1"
            ;;
        --strip)
            export ENABLE_STRIP=1
            ;;
        --no-strip)
            export ENABLE_STRIP=0
            ;;
        --no-build)
            NO_BUILD=1
            ;;
         --analysis)
            ANALYSIS=1
            ;;
        --no-unpack)
            NO_UNPACK=1
            ;;
        --bullseye|--bulleye)
            BULLSEYE=1
            ;;
        --covfile)
            shift
            COVFILE=$1
            ;;
        --bullseye-system-tests)
            BULLSEYE=1
            BULLSEYE_UPLOAD=1
            BULLSEYE_SYSTEM_TESTS=1
            COVFILE="/tmp/root/sspl-base-combined.cov"
            #ToDo remove the above LINUXDAR-1816
            ;;
        --bullseye-system-test-selector)
            shift
            export TEST_SELECTOR="$1"
            ;;
        --bullseye-upload-unittest|--bullseye-upload)
            BULLSEYE_UPLOAD=1
            ;;
        --bullseye-system-test-branch)
            shift
            BULLSEYE_SYSTEM_TEST_BRANCH=$1
            ;;
        --python-coverage)
            PythonCoverage="ON"
            ;;
        -j|--parallel)
            shift
            NPROC=$1
            ;;
        -j*)
            NPROC=${1#-j}
            ;;
        --parallel-test)
            shift
            TEST_NPROC=$1
            ;;
        --valgrind)
            VALGRIND=1
            ;;
        --unittest)
            UNIT_TESTS=1
            ;;
        --no-unittest|--no-unittests)
            UNIT_TESTS=0
            ;;
        --strace|--strace-support)
            STRACE_SUPPORT="ON"
            ;;
        --setup)
            python3 -m build_scripts.artisan_fetch build/release-package.xml
            # delete gcc
            DELETE_GCC=1
            NO_BUILD=1
            ;;
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done

function build()
{
    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "Initial PATH=$PATH"
    echo "Initial LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"
    echo "Build type=${CMAKE_BUILD_TYPE}"
    echo "Debug=${DEBUG}"

    if [[ ! -d "$FETCHED_INPUTS_DIR" ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi



    echo "After setup: PATH=$PATH"
    echo "After setup: LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"
    echo "After setup: LIBRARY_PATH=${LIBRARY_PATH}"
    echo "After setup: CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}"
    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"

    [[ $CLEAN == 1 ]] && rm -rf "$BASE/$DEBUG_BUILD_DIR"
    [[ $CLEAN == 1 ]] && rm -rf "$BASE/$RELEASE_BUILD_DIR"
    [[ $CLEAN == 1 ]] && rm -rf $OUTPUT

#    # Run static analysis
#    if [[ $ANALYSIS == 1 ]]
#    then
#      cppcheck_build  build${BITS} || exitFailure $FAILURE_CPPCHECK "Cppcheck static analysis build failed: $?"
#    fi
#
    if [[ "${NO_BUILD}" == "1" ]]
    then
        exit 0
    fi

    if [[ "$CMAKE_BUILD_TYPE" == "RelWithDebInfo" ]]
    then
      echo "Build type is release"
      BUILD_DIR="$BASE/$RELEASE_BUILD_DIR"
    elif [[ "$CMAKE_BUILD_TYPE" == "Debug" ]]
    then
      echo "Build type is debug"
      BUILD_DIR="$BASE/$DEBUG_BUILD_DIR"
    else
      # https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
      exitFailure $FAILURE_BAD_ARGUMENT "CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE but should be either Debug or RelWithDebInfo"
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
#    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >env
#    echo "export PATH=$PATH" >>env

    [[ -n ${NPROC:-} ]] || { which nproc > /dev/null 2>&1 && NPROC=$((`nproc`)); } || NPROC=2
    (( $NPROC < 1 )) && NPROC=1
    cmake -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
          .. \
          || exitFailure 14 "Failed to configure $PRODUCT"
#    cmake -v \
#        -DPRODUCT_NAME="${PRODUCT_NAME}" \
#        -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
#        -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
#        -DREDIST="${REDIST}" \
#        -DINPUT="${REDIST}" \
#        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
#        -DNO_GCOV="true" \
#        -DPythonCoverage="${PythonCoverage}" \
#        -DSTRACE_SUPPORT="${STRACE_SUPPORT}" \
#        .. \
#        || exitFailure 14 "Failed to configure $PRODUCT"


    # TODO - MOVE TO CMAKE
    # TODO LINUXDAR-1506: remove the patching when the related issue is incorporated into the released version of boost
    # https://github.com/boostorg/process/issues/62
    BOOST_PROCESS_TARGET=${REDIST}/boost/include/boost/process/detail/posix/executor.hpp
    diff -u patched_boost_executor.hpp ${BOOST_PROCESS_TARGET} && DIFFERS=0 || DIFFERS=1
    if [[ "${DIFFERS}" == "1" ]]; then
      echo "Patch Boost executor"
      cp "$BASE/patched_boost_executor.hpp"  ${BOOST_PROCESS_TARGET}
    else
      echo 'Boost executor already patched'
    fi


#    make -j${NPROC} copy_libs || exitFailure 15 "Failed to build $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"


#    if (( ${UNIT_TESTS} == 1 ))
#    then
#        if (( ${VALGRIND} == 1 ))
#        then
#            ## -VV --debug
#            export NPROC
#            bash ${BASE}/build/valgrind/runValgrind.sh \
#             || {
#                local EXITCODE=$?
#                exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
#            }
#            echo 'Valgrind test finished'
#            exit 0
#
        [[ -n ${TEST_NPROC:-} ]] || TEST_NPROC=$NPROC
#        elif (( ${BULLSEYE_SYSTEM_TESTS} == 0 ))
#        then
            ## If we are doing bullseye system tests then don't run unit test first
            ## Otherwise run the unit-tests now
            timeout 310s ctest \
                --parallel ${TEST_NPROC} \
                --test-action test \
                --no-compress-output --output-on-failure \
                --timeout 300 \
                || {
                  local EXITCODE=$?
                  echo "Unit tests failed with $EXITCODE"
                  cat Testing/Temporary/LastTest.log || true
                  cat /tmp/unitTest.log || true
                  exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
                }

            cat /tmp/unitTest.log || true

#        fi

        # python would have been executed in unit tests creating pyc and or pyo files
        # need to make sure these are removed before creating distribution files.
#        find "$BUILD_DIR" -name "*.pyc" -type f
        find "$BUILD_DIR" -name "*.pyc" -type f | xargs -r rm -f
        find "$BUILD_DIR" -name "*.pyo" -type f | xargs -r rm -f
#    fi
    make -j${NPROC} install || exitFailure 17 "Failed to install $PRODUCT"
#    make dist || exitFailure 18 "Failed to create distribution"
#    cd ..

    # TODO - do this in cmake and also save on having two copies of distribution / output
    echo "Generating output dir: $OUTPUT"
    mkdir -p $OUTPUT
    echo "STARTINGDIR=$STARTINGDIR" >$OUTPUT/STARTINGDIR
    echo "BASE=$BASE" >$OUTPUT/BASE
    echo "PATH=$PATH" >$OUTPUT/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >$OUTPUT/LD_LIBRARY_PATH
    rm -rf $OUTPUT/SDDS-COMPONENT
    cp -a "$BUILD_DIR/distribution/" "$OUTPUT/SDDS-COMPONENT" || exitFailure 21 "Failed to copy SDDS package: $?"
    cp -a "$BUILD_DIR/products/PluginApi/pluginapi.tar.gz" "$OUTPUT/pluginapi.tar.gz" || exitFailure 22 "Failed to copy pluginapi.tar.gz package: $?"
    pushd "$BUILD_DIR"
      tar -zcvf "$OUTPUT/SystemProductTestOutput.tar.gz" SystemProductTestOutput/ || exitFailure 23 "Failed to tar SystemProductTestOutput package: $?"
    popd

    if [[ -d $BUILD_DIR/symbols ]]
    then
        cp -a $BUILD_DIR/symbols output/
    fi

#    if [[ ${BULLSEYE} == 1 ]]
#    then
#      ## upload unit tests
#      if [[ ${UNIT_TESTS} == 1 ]]
#      then
#            ## Process bullseye output
#            ## upload unit tests
#            cd $BASE
#
#            #keep the local jenkins tests seperated
#            export COV_HTML_BASE=sspl-base-unittest
#            export BULLSEYE_UPLOAD
#            bash -x build/bullseye/uploadResults.sh || exit $?
#      fi
#      cp -a ${COVFILE}  output   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
#    fi
    echo "Build completed"
}

build 2>&1 | tee -a "$LOG"
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
