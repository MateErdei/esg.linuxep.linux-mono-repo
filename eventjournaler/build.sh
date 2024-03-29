#!/usr/bin/env bash
DEFAULT_PRODUCT=eventjournaler
FAILURE_UNIT_TESTS=16
FAILURE_DIST_FAILED=18
FAILURE_COPY_SDDS_FAILED=60
FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
FAILURE_COPY_SDDS_FAILED=60
FAILURE_COPY_CPPCHECK_RESULT_FAILED=61
FAILURE_CPPCHECK=62

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
[ -f "$BASE"/build-files/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-files/common.sh

CMAKE_BUILD_TYPE=RelWithDebInfo
EXTRA_CMAKE_OPTIONS=
export PRODUCT=${PLUGIN_NAME:-${DEFAULT_PRODUCT}}
export FEATURE_LIST=CORE
export PRODUCT_NAME=SPL-Event-Journaler-Plugin
export PRODUCT_LINE_ID=ServerProtectionLinux-Plugin-EventJournaler
export DEFAULT_HOME_FOLDER=
PLUGIN_TAR=
[[ -z "${CLEAN:-}" ]] && CLEAN=1
UNITTEST=1
export ENABLE_STRIP=1
BULLSEYE=0
BULLSEYE_UPLOAD=0
COVFILE="/tmp/root/sspl-plugin-${PRODUCT}-unit.cov"
COV_HTML_BASE=sspl-plugin-${PRODUCT}-unittest
VALGRIND=0
TAP=${TAP:-tap}
NO_BUILD=0

while [[ $# -ge 1 ]]
do
    case $1 in
        --dev)
            export ENABLE_STRIP=0
            CMAKE_BUILD_TYPE=Debug
            NO_UNPACK=1
            CLEAN=0
            ;;
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
        --valgrind)
            VALGRIND=1
            ;;
        --060)
            export VERSION_OVERRIDE=0.6.0.999
            ;;
        --setup)
            rm -rf input "${REDIST}"
            [[ -d ${BASE}/tapvenv ]] && source $BASE/tapvenv/bin/activate
            export TAP_PARAMETER_MODE=release
            $TAP fetch linux_mono_repo.plugins.ej_release
            NO_BUILD=1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

[[ -n "${PLUGIN_NAME}" ]] || PLUGIN_NAME=${DEFAULT_PRODUCT}
[[ -n "${PRODUCT}" ]] || PRODUCT=${PLUGIN_NAME}
[[ -n "${PRODUCT_NAME}" ]] || PRODUCT_NAME="Sophos Server Protection Linux - $PRODUCT"
[[ -n "${PRODUCT_LINE_ID}" ]] || PRODUCT_LINE_ID="ServerProtectionLinux-Plugin-$PRODUCT"
[[ -n "${DEFAULT_HOME_FOLDER}" ]] || DEFAULT_HOME_FOLDER="$PRODUCT"

export NO_REMOVE_GCC=1

INPUT=/build/input

if [[ ! -d "$INPUT" ]]
then
    MESSAGE_PART1="You need to run the following to setup your input folder: "
    MESSAGE_PART2="python3 -m build_scripts.artisan_fetch build-files/release-package.xml"
    exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
fi

function untar_input()
{
    local input=$1
    local tarbase=$2
    local override_tar=$3
    local output=$4
    local tar
    local dir
    if [[ -n $tarbase ]]
    then
        tar=${INPUT}/${tarbase}.tar
    else
        tar=${INPUT}/${input}.tar
    fi

    if [[ -n $output ]]
    then
        dir=$REDIST/$output
        mkdir -p $dir
    else
        dir=$REDIST
    fi

    if [[ -n "$override_tar" ]]
    then
        echo "Untaring override $override_tar"
        tar xzf "${override_tar}" -C "$dir"
    elif [[ -f "$tar" ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$dir"
    elif [[ -f "${tar}.gz" ]]
    then
        echo "untaring ${tar}.gz"
        tar xzf "${tar}.gz" -C "$dir"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

function cppcheck_build()
{
    local BUILD_BITS_DIR=$1

    [[ -d ${BUILD_BITS_DIR} ]] || mkdir -p ${BUILD_BITS_DIR}
    CURR_WD=$(pwd)
    cd ${BUILD_BITS_DIR}
    cmake "${BASE}"
    CPP_XML_REPORT="err.xml"
    CPP_REPORT_DIR="cppcheck"
    make cppcheck 2> ${CPP_XML_REPORT}
    python3 "$BASE/build/analysis/cppcheck-htmlreport.py" --file=${CPP_XML_REPORT} --report-dir=${CPP_REPORT_DIR} --source-dir=${BASE}
    ANALYSIS_ERRORS=$(grep 'severity="error"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_WARNINGS=$(grep 'severity="warning"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_PERFORMANCE=$(grep 'severity="performance"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_INFORMATION=$(grep 'severity="information"' ${CPP_XML_REPORT} | wc -l)
    ANALYSIS_STYLE=$(grep 'severity="style"' ${CPP_XML_REPORT} | wc -l)

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
    cd "${CURR_WD}"

     # Fail the build if there are any static analysis warnings or errors.
    [[ $ANALYSIS_ERRORS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_ERRORS static analysis errors"
    [[ $ANALYSIS_WARNINGS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_WARNINGS static analysis warnings"
}

function build()
{

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
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

        if [[ ! -d $REDIST/cmake ]]
        then
           ln -snf $INPUT/cmake $REDIST
        fi
        if [[ -d "$INPUT/googletest" ]]
        then
            if [[ ! -d $REDIST/googletest ]]
            then
                ln -sf $INPUT/googletest $REDIST/googletest
            fi
        else
            echo "ERROR - googletest not found here: $INPUT/googletest"
            exit 1
        fi
        untar_input JournalLib
        untar_input capnproto
    else
        (( LOCAL_GCC == 0 )) && set_gcc_make
    fi

    PATH=$REDIST/cmake/bin:$PATH
    chmod 700 $REDIST/cmake/bin/cmake || exitFailure "Unable to chmod cmake"
    chmod 700 $REDIST/cmake/bin/ctest || exitFailure "Unable to chmod ctest"
    if [[ ! -d $BASE/tests/googletest ]]
    then
      cp -r $REDIST/googletest $BASE/tests
    fi


    if [[ ${BULLSEYE} == 1 ]]
    then
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bulleye"
        PATH=${BULLSEYE_DIR}/bin:$PATH
        export LD_LIBRARY_PATH=${BULLSEYE_DIR}/lib:${LD_LIBRARY_PATH}
        export COVFILE
        export COV_HTML_BASE
        export BULLSEYE_DIR
        bash -x "$BASE/build/bullseye/createCovFile.sh" || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
        covclear || exitFailure $FAILURE_BULLSEYE "Unable to clear results"
    fi

    #   Required for build scripts to run on dev machines
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

    # Run static analysis
    if [[ $ANALYSIS == 1 ]]
    then
      cppcheck_build  build64 || exitFailure $FAILURE_CPPCHECK "Cppcheck static analysis build failed: $?"
    fi

    if (( $NO_BUILD == 1 ))
    then
        exit 0
    fi

    [[ $CLEAN == 1 ]] && rm -rf build64
    mkdir -p build64
    cd build64
    [[ -n ${NPROC:-} ]] || NPROC=$(nproc || echo 2)
    cmake -DREDIST="${REDIST}" \
             -DINPUT="${REDIST}" \
            -DPLUGIN_NAME="${PLUGIN_NAME}" \
            -DPRODUCT_NAME="${PRODUCT_NAME}" \
            -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
            -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
            -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
            -DCMAKE_CXX_COMPILER=$CXX \
            -DCMAKE_C_COMPILER=$CC \
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
    elif (( ${UNITTEST} == 1 ))
    then
        ctest \
          --parallel ${NPROC} \
          --timeout 15 \
          --output-on-failure \ || {
            local EXITCODE=$?
            echo "Unit tests failed with $EXITCODE"
#            cat Testing/Temporary/LastTest.log || true
            exitFailure $FAILURE_UNIT_TESTS "Unit tests failed for $PRODUCT"
        }
    fi
    make install CXX=$CXX CC=$CC || exitFailure 17 "Failed to install $PRODUCT"
    make dist_sdds CXX=$CXX CC=$CC ||  exitFailure $FAILURE_DIST_FAILED "Failed to create dist $PRODUCT"
    cd ..

    rm -rf output
    mkdir -p output
    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    [[ -f build64/sdds/SDDS-Import.xml ]] || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to create SDDS-Import.xml"
    cp -a build64/sdds output/SDDS-COMPONENT || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS component to output"

    mkdir -p  output/manualTools/
    cp -a build64/products/manualTools/EventPubSub output/manualTools/ || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy EventPubSub Tool to output"
    cp -a build64/products/manualTools/EventJournalWriter output/manualTools/ || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy EventJournalWriter Tool to output"
    cp -a ${INPUT}/JournalReader output/manualTools/ || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy JournalReader Tool to output"
    cp -a ${INPUT}/base-sdds  output/base-sdds  || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy base SDDS component to output"
    cp -a ${INPUT}/fake-management  output/fake-management  || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy fake management to output"
    if [[ -d build64/symbols ]]
    then
        cp -a build64/symbols output/
    fi

    if [[ ${BULLSEYE} == 1 ]]
    then
      if [[ ${UNITTEST} == 1 ]]
      then
        ## Process bullseye output
        ## upload unit tests
        cd $BASE

        #keep the local jenkins tests seperated
        export COV_HTML_BASE=sspl-plugin-eventjournaler-unittest
        export BULLSEYE_UPLOAD
        bash -x build/bullseye/uploadResults.sh || exit $?
        fi
      cp -a ${COVFILE}  output   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
    fi

    echo "Build Successful"
    return 0
}

build 2>&1 | tee -a $LOG
EXIT=$?
if [[ -d $OUTPUT ]]
then
  cp $LOG $OUTPUT/ || true
fi
exit $EXIT
