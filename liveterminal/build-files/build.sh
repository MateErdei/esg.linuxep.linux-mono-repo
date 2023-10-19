#!/usr/bin/env bash
DEFAULT_PRODUCT=liveresponse

FAILURE_DIST_FAILED=18
FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
FAILURE_UNIT_TESTS=54
FAILURE_BUILD_LIVETERMINAL_AGENT=55
FAILURE_COPY_SDDS_FAILED=60
FAILURE_COPY_CPPCHECK_RESULT_FAILED=61
FAILURE_CPPCHECK=62

set -ex
set -o pipefail

SRCROOT=$(pwd)
export SRCROOT
BUILD_FILES="${SRCROOT}/build-files"
OUTPUT=${SRCROOT}/output

LOG=${SRCROOT}/log/build.log
mkdir -p ${SRCROOT}/log || exit 1

# shellcheck disable=SC1090
source "${BUILD_FILES}/common.sh"

VERSION=$(grep "<package name=\"liveterminal_linux\"" ${SRCROOT}/linux-release-package.xml | sed -e s/.*version=\"// -e s/\"\>//  -e s/-/\./g )

CMAKE_BUILD_TYPE=RelWithDebInfo
EXTRA_CMAKE_OPTIONS=
export PRODUCT=${PLUGIN_NAME:-${DEFAULT_PRODUCT}}
export PRODUCT_NAME=
export PRODUCT_LINE_ID=
export DEFAULT_HOME_FOLDER=
PLUGIN_TAR=
[[ -z "${CLEAN:-}" ]] && CLEAN=1
UNITTEST=1
export ENABLE_STRIP=1
BULLSEYE=0
BULLSEYE_UPLOAD=0
ANALYSIS=0
COVFILE="${SRCROOT}/liveterminal_unittests.cov"

while [[ $# -ge 1 ]]
do
    case $1 in
        --build-type)
            shift
            CMAKE_BUILD_TYPE="$1"
            ;;
        --debug)
            export ENABLE_STRIP=0
            CMAKE_BUILD_TYPE=Debug
            ;;
        --no-debug)
            export ENABLE_STRIP=1
            CMAKE_BUILD_TYPE=RelWithDebInfo
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
        --no-unpack)
            NO_UNPACK=1
            ;;
        --cmake-option)
            shift
            EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} $1"
            ;;
        --plugin-api)
            shift
            EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DPLUGINAPIPATH=$1"
            ;;
        --plugin-api-tar)
            shift
            PLUGIN_TAR=$1
            ;;
        --name)
            shift
            PLUGIN_NAME=$1
            PRODUCT=$1
            ;;
        --product-name)
            shift
            PRODUCT_NAME="$1"
            ;;
        --product-line-id|--rigidname)
            shift
            PRODUCT_LINE_ID="$1"
            i;;
        --default-home-folder)
            shift
            DEFAULT_HOME_FOLDER="$1"
            ;;
        -j)
            shift
            NPROC=$1
            ;;
        -j*)
            NPROC=${1#-j}
            ;;
        --clean)
            CLEAN=1
            ;;
        --no-clean|--noclean)
            CLEAN=0
            ;;
        --analysis)
            ANALYSIS=1
            ;;
        --unit-test)
            UNITTEST=1
            ;;
        --no-unit-test)
            UNITTEST=0
            ;;
        --bullseye|--bulleye)
            BULLSEYE=1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

[[ -n "${PLUGIN_NAME}" ]] || PLUGIN_NAME=${DEFAULT_PRODUCT}
[[ -n "${PRODUCT}" ]] || PRODUCT=${PLUGIN_NAME}
[[ -n "${PRODUCT_NAME}" ]] || PRODUCT_NAME="SPL-Live-Response-Plugin"
[[ -n "${PRODUCT_LINE_ID}" ]] || PRODUCT_LINE_ID="ServerProtectionLinux-Plugin-${PRODUCT}"
[[ -n "${DEFAULT_HOME_FOLDER}" ]] || DEFAULT_HOME_FOLDER="$PRODUCT"

export NO_REMOVE_GCC=1

INPUT=/build/input

if [[ ! -d "$INPUT" ]]
then
    MESSAGE_PART1="You need to run the following to setup your input folder: "
    MESSAGE_PART2="tap fetch  liveterminallinux --build-backend=local"
    exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
fi

function untar_input()
{
    local input=$1
    local tarbase=$2
    local override_tar=$3
    local tar
    if [[ -n $tarbase ]]
    then
        tar=${INPUT}/${tarbase}.tar
    else
        tar=${INPUT}/${input}.tar
    fi

    if [[ -n "$override_tar" ]]
    then
        echo "Untaring override $override_tar"
        tar xzf "${override_tar}" -C "$REDIST"
    elif [[ -f "$tar" ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    elif [[ -f "${tar}.gz" ]]
    then
        echo "untaring ${tar}.gz"
        tar xzf "${tar}.gz" -C "$REDIST"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

function cppcheck_build() {
    local BUILD_BITS_DIR=$1
    [[ -d ${BUILD_BITS_DIR} ]] || mkdir -p ${BUILD_BITS_DIR}

    pushd ${BUILD_BITS_DIR}
    ${CMAKE} "${SRCROOT}"
    CPP_XML_REPORT="err.xml"
    CPP_REPORT_DIR="cppcheck"
    make cppcheck 2> ${CPP_XML_REPORT}
    python3 "${BUILD_FILES}"/cppcheck-htmlreport.py --file=${CPP_XML_REPORT} --report-dir=${CPP_REPORT_DIR} --source-dir=${SRCROOT}

    set +o pipefail
    ANALYSIS_ERRORS=$(grep 'severity="error"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_WARNINGS=$(grep 'severity="warning"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_PERFORMANCE=$(grep 'severity="performance"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_INFORMATION=$(grep 'severity="information"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_STYLE=$(grep 'severity="style"' ${CPP_XML_REPORT} | wc -l)
    set -o pipefail
    echo "The full XML static analysis report:"
    cat ${CPP_XML_REPORT}
    echo "There are $ANALYSIS_ERRORS static analysis error issues"
    echo "There are $ANALYSIS_WARNINGS static analysis warning issues"
    echo "There are $ANALYSIS_PERFORMANCE static analysis performance issues"
    echo "There are $ANALYSIS_INFORMATION static analysis information issues"
    echo "There are $ANALYSIS_STYLE static analysis style issues"

    ANALYSIS_OUTPUT_DIR="${OUTPUT}/analysis/"
    [[ -d ${ANALYSIS_OUTPUT_DIR} ]] || mkdir -p "${ANALYSIS_OUTPUT_DIR}"
    cp -a ${CPP_REPORT_DIR}  "${ANALYSIS_OUTPUT_DIR}" || exitFailure $FAILURE_COPY_CPPCHECK_RESULT_FAILED  "Failed to copy cppcheck report to output"
    popd

    # Fail the build if there are any static analysis warnings or errors.
    [[ $ANALYSIS_ERRORS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_ERRORS static analysis errors"
    [[ $ANALYSIS_WARNINGS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_WARNINGS static analysis warnings"
}

function build()
{
    local BITS=$1

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=${SRCROOT}"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    if [[ ! -d $INPUT ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi

    REDIST=/build/redist

    if [[ -z "$NO_UNPACK" ]]
    then
        mkdir -p $REDIST
        unpack_scaffold_gcc_make "$INPUT"
        untar_input pluginapi "" ${PLUGIN_TAR}
        if [[ ! -d ${REDIST}/cmake ]]
        then
          cp -r ${INPUT}/cmake ${REDIST}
        fi


        untar_input protobuf
    fi

    export CMAKE=${REDIST}/cmake/bin/cmake
    chmod 700 $REDIST/cmake/bin/cmake || exitFailure "Unable to chmod cmake"
    chmod 700 $REDIST/cmake/bin/ctest || exitFailure "Unable to chmod ctest"

    chmod +x ${REDIST}/sdds3/*

    if [[ ${BULLSEYE} == 1 ]]
    then
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bullseye"
        export PATH=${BULLSEYE_DIR}/bin:$PATH
        export LD_LIBRARY_PATH=${BULLSEYE_DIR}/lib:${LD_LIBRARY_PATH}
        export COVFILE
        export COV_HTML_BASE
        export BULLSEYE_DIR
        bash -x "${BUILD_FILES}/createCovFile.sh" "${SRCROOT}" || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
        covclear || exitFailure $FAILURE_BULLSEYE "Unable to clear results"
    else
        export CC=/build/input/gcc/bin/gcc
        export CXX=/build/input/gcc/bin/g++
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/build/input/gcc/lib64/
    fi

    # Required for build scripts to run on dev machines
    export LIBRARY_PATH=/build/input/gcc/lib64/:/usr/lib/x86_64-linux-gnu/:${LIBRARY_PATH}
    export CPLUS_INCLUDE_PATH=/build/input/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}
    export C_INCLUDE_PATH=/build/input/gcc/include/:/usr/include/x86_64-linux-gnu/:${C_INCLUDE_PATH}

    echo "After setup: LIBRARY_PATH=${LIBRARY_PATH}"
    echo "After setup: CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}"
    echo "After setup: C_INCLUDE_PATH=${C_INCLUDE_PATH}"

    [[ -n $CC ]] || CC=$(which gcc)
    [[ -n $CXX ]] || CXX=$(which g++)
    export CC
    export CXX

    [[ $CLEAN == 1 ]] && rm -rf $OUTPUT

    if [[ $ANALYSIS == 1 ]]
    then
      cppcheck_build build${BITS}
    fi

    if [[ $NO_BUILD == 1 ]]
    then
        exit 0
    fi

    RustAgentMode="release"
    if [ "${CMAKE_BUILD_TYPE,,}" = "debug" ]; then
        RustAgentMode=debug
    fi
    RustCoverage="off"
    if [[ ${BULLSEYE} == 1 ]]
    then
      RustCoverage="on"
    fi


    LiveTerminalAgent=/build/redist/rust-liveterminal/sophos-live-terminal
    if [[ ! -f ${LiveTerminalAgent} ]]; then
        exitFailure $FAILURE_BUILD_LIVETERMINAL_AGENT "No liveterminal agent at: ${LiveTerminalAgent}"
    fi

     MODE="RELEASE"
    if [[ ${BULLSEYE} == 1 ]]
    then
      MODE="COVERAGE"
    fi

    [[ $CLEAN == 1 ]] && rm -rf build${BITS}
    mkdir -p build${BITS}
    cd build${BITS}
    [[ -n ${NPROC:-} ]] || NPROC=2
    ${CMAKE} -DREDIST="${REDIST}" \
            -DLR_PLUGIN_NAME="${PLUGIN_NAME}" \
            -DMODE="${MODE}"\
            -DLR_PRODUCT_NAME="${PRODUCT_NAME}" \
            -DLR_PRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
            -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
            -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
            -DCMAKE_CXX_COMPILER=$CXX \
            -DCMAKE_C_COMPILER=$CC \
            -DLIVETERMINAL=${LiveTerminalAgent} \
            -DNO_GCOV="true" \
            ${EXTRA_CMAKE_OPTIONS} \
        ${SRCROOT} || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} CXX=$CXX CC=$CC || exitFailure 15 "Failed to build $PRODUCT"

    if (( ${UNITTEST} == 1 ))
    then
        make -j${NPROC} CTEST_OUTPUT_ON_FAILURE=1  test || {
            local EXITCODE=$?
            echo "Unit tests failed with $EXITCODE"
            cat Testing/Temporary/LastTest.log || true
            exitFailure $FAILURE_UNIT_TESTS "Unit tests failed for $PRODUCT"
        }
    fi
    make install CXX=$CXX CC=$CC || exitFailure 17 "Failed to install $PRODUCT"
    make dist CXX=$CXX CC=$CC ||  exitFailure $FAILURE_DIST_FAILED "Failed to create dist $PRODUCT"
    cd ..

    rm -rf output
    mkdir -p output
    echo "STARTINGDIR=${SRCROOT}" >output/STARTINGDIR
    echo "BASE=${SRCROOT}" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    [[ -f build64/sdds/SDDS-Import.xml ]] || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to create SDDS-Import.xml"
    cp -a build64/sdds output/SDDS-COMPONENT || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS component to output"
    cp -a build64/sdds/SDDS-Import.xml output/SDDS3-PACKAGE || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS-Import.xml to SDDS3-PACKAGE output"

    if [[ ${BULLSEYE} == 1 ]]
    then
      COVTARGET="output/coverage/covfile"
      mkdir -p ${COVTARGET}
      cp -a ${COVFILE}  ${COVTARGET}/liveterminal_unittests.cov   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
      covhtml --no-banner ${COVTARGET}/../bullseye_report
      covclear --no-banner
      cp -a ${COVFILE}  ${COVTARGET}/liveterminal_empty.cov   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
    fi

    if [[ -d build${BITS}/symbols ]]
    then
        cp -a build${BITS}/symbols output/
    fi

    echo "Build Successful"
    return 0
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
