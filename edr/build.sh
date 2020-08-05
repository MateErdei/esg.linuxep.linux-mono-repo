#!/usr/bin/env bash
DEFAULT_PRODUCT=edr

FAILURE_DIST_FAILED=18
FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
FAILURE_UNIT_TESTS=54
FAILURE_COPY_SDDS_FAILED=60

source /etc/profile
set -ex
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
export BASE
OUTPUT=$BASE/output
export OUTPUT

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

## These can't be exitFailure since it doesn't exist till the sourcing is done
[ -f "$BASE"/build-files/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
source "$BASE"/build-files/pathmgr.sh
[ -f "$BASE"/build-files/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-files/common.sh

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
BULLSEYE_COMPONENT_TESTS=0
COVFILE="/tmp/root/sspl-plugin-${PRODUCT}-unit.cov"
COV_HTML_BASE=sspl-plugin-edr-unittest
VALGRIND=0
GOOGLETESTTAR=googletest-release-1.8.1

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
        --analysis)
            NO_BUILD=1
            ANALYSIS=1
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
            ;;
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
        --unit-test)
            UNITTEST=1
            ;;
        --no-unit-test)
            UNITTEST=0
            ;;
        --bullseye|--bulleye)
            BULLSEYE=1
            ;;
        --bullseye-upload-unittest|--bullseye-upload)
            BULLSEYE_UPLOAD=1
            ;;
        --bullseye-system-tests)
            BULLSEYE=1
            BULLSEYE_COMPONENT_TESTS=1
            COVFILE="/tmp/root/sspl-edr-combined.cov"
            COV_HTML_BASE=sspl-plugin-edr-combined
            ;;
        --valgrind)
            VALGRIND=1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

[[ -n "${PLUGIN_NAME}" ]] || PLUGIN_NAME=${DEFAULT_PRODUCT}
[[ -n "${PRODUCT}" ]] || PRODUCT=${PLUGIN_NAME}
[[ -n "${PRODUCT_NAME}" ]] || PRODUCT_NAME="Sophos Endpoint Detection and Response plug-in"
[[ -n "${PRODUCT_LINE_ID}" ]] || PRODUCT_LINE_ID="ServerProtectionLinux-Plugin-EDR"
[[ -n "${DEFAULT_HOME_FOLDER}" ]] || DEFAULT_HOME_FOLDER="$PRODUCT"

export NO_REMOVE_GCC=1

INPUT=$BASE/input

if [[ ! -d "$INPUT" ]]
then
    if [[ -d "$BASE/sspl-plugin-edr-component-build" ]]
    then
        INPUT="$BASE/sspl-plugin-edr-component-build/input"
    else
        MESSAGE_PART1="You need to run the following to setup your input folder: "
        MESSAGE_PART2="python3 -m build_scripts.artisan_fetch build-files/release-package.xml"
        exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
    fi
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

#    yum -y install cppcheck
#    yum -y install python36-pygments

    [[ -d ${BUILD_BITS_DIR} ]] || mkdir -p ${BUILD_BITS_DIR}
    cd ${BUILD_BITS_DIR}
    cmake -v "${BASE}"
    CPP_XML_REPORT="err.xml"
    CPP_REPORT_DIR="cppcheck"
    make cppcheck 2> ${CPP_XML_REPORT}
    python3 "$BASE/build/analysis/cppcheck-htmlreport.py" --file=${CPP_XML_REPORT} --report-dir=${CPP_REPORT_DIR} --source-dir=${BASE}

    ANALYSIS_OUTPUT_DIR="${OUTPUT}/coverage/"
    [[ -d ${ANALYSIS_OUTPUT_DIR} ]] || mkdir -p ${ANALYSIS_OUTPUT_DIR}
    cp -a ${CPP_REPORT_DIR}  ${ANALYSIS_OUTPUT_DIR} || exitFailure $FAILURE_COPY_CPPCHECK_RESULT_FAILED  "Failed to copy cppcheck report to output"
}

function build()
{
    local BITS=$1

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    if [[ ! -d $INPUT ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi

    REDIST=$BASE/redist

    if [[ -z "$NO_UNPACK" ]]
    then
        mkdir -p $REDIST
        unpack_scaffold_gcc_make "$INPUT"
        untar_input pluginapi "" ${PLUGIN_TAR}
        untar_input cmake cmake-3.11.2-linux
        untar_input $GOOGLETESTTAR
        untar_input boost
        untar_input osquerysdk
        untar_input openssl
        untar_input protobuf

        mkdir -p "$REDIST"/osquery
        tar xzf ${INPUT}/osquery-4.4.0_1.linux_x86_64.tar.gz -C "$REDIST"/osquery

    fi

    addpath "$REDIST/cmake/bin"
    cp -r $REDIST/$GOOGLETESTTAR $BASE/tests/googletest

    if [[ ${BULLSEYE} == 1 ]]
    then
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bulleye"
        addpath ${BULLSEYE_DIR}/bin:$PATH
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

    if [[ $NO_BUILD == 1 ]]
    then
      if [[ $ANALYSIS == 1 ]]
      then
        cppcheck_build  build${BITS} || {
            EXIT=$?
            echo " Cppcheck static analysis build failed: $EXIT"
            exit ${EXIT}
        }
      fi
        exit 0
    fi


    [[ $CLEAN == 1 ]] && rm -rf build${BITS}
    mkdir -p build${BITS}
    cd build${BITS}
    [[ -n ${NPROC:-} ]] || NPROC=$(nproc) ||  [[ -n ${NPROC:-} ]] || NPROC=2
    cmake -v -DREDIST="${REDIST}" \
             -DINPUT="${REDIST}" \
            -DPLUGIN_NAME="${PLUGIN_NAME}" \
            -DPRODUCT_NAME="${PRODUCT_NAME}" \
            -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
            -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
            -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
            -DCMAKE_CXX_COMPILER=$CXX \
            -DCMAKE_C_COMPILER=$CC \
            -DNO_GCOV="true" \
            ${EXTRA_CMAKE_OPTIONS} \
        .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} CXX=$CXX CC=$CC || exitFailure 15 "Failed to build $PRODUCT"

    if (( ${VALGRIND} == 1 ))
    then
        ## -VV --debug
        ctest \
        --test-action memcheck --parallel ${NPROC} \
        --output-on-failure \
         || {
            local EXITCODE=$?
            exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
        }
    # Run the unit tests unless we are doing bullseye system tests then don't run unit test first
    elif (( ${UNITTEST} == 1 ))
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
    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    [[ -f build64/sdds/SDDS-Import.xml ]] || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to create SDDS-Import.xml"
    cp -a build64/sdds output/SDDS-COMPONENT || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS component to output"
    cp -a ${INPUT}/base-sdds  output/base-sdds  || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy base SDDS component to output"
    cp -a build64/componenttests output/componenttests    || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy google component tests"

    if [[ ${BULLSEYE} == 1 ]]
    then
      #move the coverage build produced and replace sdds with previous last good build
      mv output/SDDS-COMPONENT output/SDDS-COMPONENT-COVERAGE || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to move coverage SDDS component to output"
      cp -a ${INPUT}/edr-sdds  output/SDDS-COMPONENT  || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to replace base SDDS component with previous good build in output"
      if [[ ${UNITTEST} == 1 ]]
      then
            ## Process bullseye output
            ## upload unit tests
            cd $BASE

            #keep the local jenkins tests seperated
            export COV_HTML_BASE=sspl-plugin-edr-unittest
            export BULLSEYE_UPLOAD
            bash -x build/bullseye/uploadResults.sh || exit $?
        fi
      cp -a ${COVFILE}  output   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
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
