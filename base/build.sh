#!/bin/bash
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

## These can't be exitFailure since it doesn't exist till the sourcing is done
[ -f "$BASE"/build/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build/common.sh

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

PythonCoverage="OFF"
STRACE_SUPPORT="OFF"
CLEAN=0
BULLSEYE=0
BULLSEYE_UPLOAD=0
BULLSEYE_SYSTEM_TESTS=0
export NO_REMOVE_GCC=1
INPUT=$BASE/input

COVFILE="/tmp/root/sspl-unit.cov"
COV_HTML_BASE=sspl-unittest
BULLSEYE_SYSTEM_TEST_BRANCH=develop
export TEST_SELECTOR=
CMAKE_BUILD_TYPE=RelWithDebInfo
DEBUG=0
export ENABLE_STRIP=1
VALGRIND=0
UNIT_TESTS=1
GOOGLETESTTAR=googletest-release-1.8.1

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
        --cov-html)
            shift
            COV_HTML_BASE=$1
            ;;
        --bullseye-system-tests)
            BULLSEYE=1
            BULLSEYE_UPLOAD=1
            BULLSEYE_SYSTEM_TESTS=1
            COVFILE="/tmp/root/sspl-combined.cov"
            COV_HTML_BASE=sspl-functional
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
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done

if [[ ! -d "$INPUT" ]]
then
    if [[ -d "$BASE/sspl-base-build" ]]
    then
        INPUT="$BASE/sspl-base-build/input"
    else
        MESSAGE_PART1="You need to run the following to setup your input folder: "
        MESSAGE_PART2="python3 -m build_scripts.artisan_fetch build/release-package.xml"
        exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
    fi
fi

case ${CMAKE_BUILD_TYPE} in
    Debug|DEBUG)
        DEBUG=1
        export ENABLE_STRIP=0
        ;;
esac

function untar_input()
{
    local input=$1
    local tar=${INPUT}/${input}.tar
    local tarzip=${INPUT}/${input}.tar.gz

    if [[ -f "$tar" ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    elif [[ -f "$tarzip" ]]
    then
        echo "Untaring $tarzip"
        tar xzf "$tarzip" -C "$REDIST"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

function cppcheck_build() {
    local BUILD_BITS_DIR=$1

    [[ -d ${BUILD_BITS_DIR} ]] || mkdir -p ${BUILD_BITS_DIR}
    CURR_WD=$(pwd)
    cd ${BUILD_BITS_DIR}
    cmake "${BASE}"
    CPP_XML_REPORT="err.xml"
    CPP_REPORT_DIR="cppcheck"
    make cppcheck 2> ${CPP_XML_REPORT}
    python3 "$BASE/build/analysis/cppcheck-htmlreport.py" --file=${CPP_XML_REPORT} --report-dir=${CPP_REPORT_DIR} --source-dir=${BASE}

    ANALYSIS_OUTPUT_DIR="${OUTPUT}/analysis/"
    [[ -d ${ANALYSIS_OUTPUT_DIR} ]] || mkdir -p "${ANALYSIS_OUTPUT_DIR}"
    cp -a ${CPP_REPORT_DIR}  "${ANALYSIS_OUTPUT_DIR}" || exitFailure $FAILURE_COPY_CPPCHECK_RESULT_FAILED  "Failed to copy cppcheck report to output"
    cd "${CURR_WD}"
}

function build()
{
    local BITS=$1

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "Initial PATH=$PATH"
    echo "Initial LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"
    echo "Build type=${CMAKE_BUILD_TYPE}"
    echo "Debug=${DEBUG}"

    if [[ ! -d "$INPUT" ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi

    REDIST=$BASE/redist

    if [[ -z "$NO_UNPACK" ]]
    then
        unpack_scaffold_gcc_make "$INPUT"

        mkdir -p $REDIST

        OPENSSL_TAR=${INPUT}/openssl.tar
        if [[ -f $OPENSSL_TAR ]]
        then
            rm -rf $REDIST/openssl
            tar xf "$OPENSSL_TAR" -C "$REDIST"
        else
            exitFailure 12 "Failed to find openssl"
        fi

        local CMAKE_TAR=$(ls $INPUT/cmake-*.tar.gz)
        if [[ -f "$CMAKE_TAR" ]]
        then
            tar xzf "$CMAKE_TAR" -C "$REDIST"
        else
            echo "WARNING: using system cmake"
        fi

        untar_input versig
        untar_input curl
        untar_input SUL
        untar_input boost
        # TODO LINUXDAR-1506: remove the patching when the related issue is incorporated into the released version of boost
        # https://github.com/boostorg/process/issues/62
        BOOST_PROCESS_TARGET=${REDIST}/boost/include/boost/process/detail/posix/executor.hpp
        diff -u patched_boost_executor.hpp ${BOOST_PROCESS_TARGET} && DIFFERS=0 || DIFFERS=1
        if [[ "${DIFFERS}" == "1" ]]; then
          echo "Patch Boost executor"
          cp patched_boost_executor.hpp  ${BOOST_PROCESS_TARGET}
        else
          echo 'Boost executor alredy patched'
        fi
        untar_input expat
        untar_input zlib
        untar_input log4cplus
        untar_input zeromq
        untar_input protobuf
        untar_input capnproto
        untar_input python
        untar_input python-watchdog
        untar_input python-pathtools
        untar_input python-certifi
        untar_input python-chardet
        untar_input python-idna
        untar_input python-requests
        untar_input python-six
        untar_input python-sseclient
        untar_input python-urllib3
        untar_input pycryptodome
        untar_input $GOOGLETESTTAR
        # below are unpacked for test use only
        untar_input gcc-8.1.0-linux

        mkdir -p ${REDIST}/certificates
        if [[ -f ${INPUT}/ps_rootca.crt ]]
        then
            cp ${INPUT}/ps_rootca.crt ${REDIST}/certificates
        else
            exitFailure $FAILURE_INPUT_NOT_AVAILABLE "ps_rootca.crt not available"
        fi

        if [[ -f ${INPUT}/manifest_certificates/rootca.crt ]]
        then
            cp "${INPUT}/manifest_certificates/rootca.crt" "${REDIST}/certificates/"
        elif [[ -f ${INPUT}/ps_rootca.crt ]]
        then
            ## Use ps_rootca.crt as rootca.crt since they are the same in practice
            cp "${INPUT}/ps_rootca.crt" "${REDIST}/certificates/rootca.crt"
        else
            exitFailure $FAILURE_INPUT_NOT_AVAILABLE "rootca.crt not available"
        fi

        if [[ -f ${INPUT}/telemetry/telemetry-config.json ]]
        then
            mkdir -p ${REDIST}/telemetry
            cp "${INPUT}/telemetry/telemetry-config.json" "${REDIST}/telemetry/"
        else
            exitFailure $FAILURE_INPUT_NOT_AVAILABLE "telemetry-config.json not available"
        fi
    fi

    #install analysis build depenecies before we mess-up with LD_LIBRARY path
    if [[ $ANALYSIS == 1 ]]
    then
      PKG_MANAGER="$( command -v yum || command -v apt-get )"
      case "${PKG_MANAGER}" in
        *yum*)
          "${PKG_MANAGER}" -y install cppcheck
          "${PKG_MANAGER}" -y install python36-pygments
        ;;
        *apt*)
          sudo "${PKG_MANAGER}" -y install python3-pygments
          sudo "${PKG_MANAGER}" -y install cppcheck
        ;;
      esac
    fi

    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib${BITS}:${REDIST}/curl/lib64:${REDIST}/log4cplus/lib:${REDIST}/zeromq/lib:${REDIST}/protobuf/install${BITS}/lib
    export PATH=${REDIST}/cmake/bin:${REDIST}/protobuf/install${BITS}/bin:${PATH}
    cp -r $REDIST/$GOOGLETESTTAR $BASE/tests/googletest


    if [[ ${BULLSEYE} == 1 ]]
    then
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bulleye"
        export PATH=${PATH}:${BULLSEYE_DIR}/bin:$PATH
        export LD_LIBRARY_PATH=${BULLSEYE_DIR}/lib:${LD_LIBRARY_PATH}
        export COVFILE
        export COV_HTML_BASE
        export BULLSEYE_DIR
        bash -x "$BASE/build/bullseye/createCovFile.sh" || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
        covclear || exitFailure $FAILURE_BULLSEYE "Unable to clear results"
    else
        export CC=/build/input/gcc/bin/gcc
        export CXX=/build/input/gcc/bin/g++
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/build/input/gcc/lib64/
    fi

#   Required for build scripts to run on dev machines
    export LIBRARY_PATH=/build/input/gcc/lib64/:${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu
    export CPLUS_INCLUDE_PATH=/build/input/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}
    export CPATH=/build/input/gcc/include/:${CPATH}

    echo "After setup: PATH=$PATH"
    echo "After setup: LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"
    echo "After setup: LIBRARY_PATH=${LIBRARY_PATH}"
    echo "After setup: CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}"
    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"

    [[ $CLEAN == 1 ]] && rm -rf build${BITS}

    #run static analysis
    if [[ $ANALYSIS == 1 ]]
    then
      cppcheck_build  build${BITS} || exitFailure $FAILURE_CPPCHECK "Cppcheck static analysis build failed: $?"
    fi

    if [[ "${NO_BUILD}" == "1" ]]
    then
        exit 0
    fi

    mkdir -p build${BITS}
    cd build${BITS}
    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >env
    echo "export PATH=$PATH" >>env

    [[ -n ${NPROC:-} ]] || { which nproc > /dev/null 2>&1 && NPROC=$((`nproc`)); } || NPROC=2
    (( $NPROC < 1 )) && NPROC=1
    cmake -v \
        -DPRODUCT_NAME="${PRODUCT_NAME}" \
        -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
        -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
        -DREDIST="${REDIST}" \
        -DINPUT="${REDIST}" \
        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
        -DNO_GCOV="true" \
        -DPythonCoverage="${PythonCoverage}" \
        -DSTRACE_SUPPORT="${STRACE_SUPPORT}" \
        .. \
        || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} copy_libs || exitFailure 15 "Failed to build $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"

    if (( ${UNIT_TESTS} == 1 ))
    then
        if (( ${VALGRIND} == 1 ))
        then
            ## -VV --debug
            export NPROC
            bash ${BASE}/build/valgrind/runValgrind.sh \
             || {
                local EXITCODE=$?
                exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
            }
            echo 'Valgrind test finished'
            exit 0
        elif (( ${BULLSEYE_SYSTEM_TESTS} == 0 ))
        then
            ## If we are doing bullseye system tests then don't run unit test first
            ## Otherwise run the unit-tests now
            ctest \
                --test-action test \
                --parallel ${NPROC} \
                --no-compress-output --output-on-failure \
                --timeout 300 \
                || {
                local EXITCODE=$?
                echo "Unit tests failed with $EXITCODE"
                cat Testing/Temporary/LastTest.log || true
                exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
            }
        fi

        # python would have been executed in unit tests creating pyc and or pyo files
        # need to make sure these are removed before creating distribution files.
        find ../build${BITS} -name "*.pyc" -type f | xargs -r rm -f
        find ../build${BITS} -name "*.pyo" -type f | xargs -r rm -f
    fi
    make -j${NPROC} install || exitFailure 17 "Failed to install $PRODUCT"
    make dist || exitFailure 18 "Failed to create distribution"
    cd ..

    mkdir -p output
    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    rm -rf output/SDDS-COMPONENT
    cp -a build${BITS}/distribution/ output/SDDS-COMPONENT || exitFailure 21 "Failed to copy SDDS package: $?"
    cp -a build${BITS}/products/PluginApi/pluginapi.tar.gz output/pluginapi.tar.gz || exitFailure 22 "Failed to copy pluginapi.tar.gz package: $?"
    pushd build${BITS}
    tar -zcvf ../output/SystemProductTestOutput.tar.gz SystemProductTestOutput/ || exitFailure 23 "Failed to tar SystemProductTestOutput package: $?"
    popd

    if [[ -d build${BITS}/symbols ]]
    then
        cp -a build${BITS}/symbols output/
    fi

    if (( ${BULLSEYE_SYSTEM_TESTS} == 1 ))
    then
        cd $BASE
        export BULLSEYE_SYSTEM_TEST_BRANCH
        bash -x $BASE/build/bullseye/runSystemTest.sh || {
            ## System tests failed to sync or similar
            EXIT=$?
            echo "System tests failed: $EXIT"
            exit $EXIT
        }
    fi

    if [[ ${BULLSEYE_UPLOAD} == 1 ]]
    then
        ## Process bullseye output
        ## upload unit or functional tests
        cd $BASE
        bash -x build/bullseye/uploadResults.sh || exit $?
    fi

    if (( ${BULLSEYE_SYSTEM_TESTS} == 1 )) && (( ${UNIT_TESTS} == 1 ))
    then
        ## Now generate combined results
        cd ${BASE}
        cd build${BITS}
        export COV_HTML_BASE=sspl-combined
        make CTEST_OUTPUT_ON_FAILURE=1 test || echo "Unit tests failed for $PRODUCT: $?"

        ## Upload combined results
        cd ${BASE}
        bash -x build/bullseye/uploadResults.sh || exit $?
    fi

    echo "Build completed"
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
