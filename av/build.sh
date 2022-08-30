#!/usr/bin/env bash
DEFAULT_PRODUCT=av

FAILURE_DIST_FAILED=18
FAILURE_COPY_SDDS_FAILED=60
FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
FAILURE_UNIT_TESTS=54
FAILURE_CPPCHECK=62
FAILURE_COPY_CPPCHECK_RESULT_FAILED=63


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
# shellcheck source=build-files/pathmgr.sh
source "$BASE"/build-files/pathmgr.sh
[ -f "$BASE"/build-files/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
# shellcheck source=build-files/common.sh
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
COVFILE="/tmp/root/sspl-plugin-${PRODUCT}.cov"
COVFILE_UNIT="sspl-plugin-${PRODUCT}-unit.cov"
COV_HTML_BASE=sspl-plugin-${PRODUCT}-unittest-dev
VALGRIND=0
NO_BUILD=0
LOCAL_GCC=0
LOCAL_CMAKE=0
DUMP_LAST_TEST_ON_FAILURE=0
RUN_CPPCHECK=0
BUILD_SDDS3=1
TAP=${TAP:-tap}

while [[ $# -ge 1 ]]
do
    case $1 in
        --ci)
            BULLSEYE=0
            LOCAL_CMAKE=0
            LOCAL_GCC=0
            NO_BUILD=0
            NO_UNPACK=
            VALGRIND=0
            CMAKE_BUILD_TYPE=RelWithDebInfo
            DUMP_LAST_TEST_ON_FAILURE=1
            BUILD_SDDS3=1
            export ENABLE_STRIP=1
            ;;
        --build-type)
            shift
            CMAKE_BUILD_TYPE="$1"
            ;;
        --debug)
            export ENABLE_STRIP=0
            CMAKE_BUILD_TYPE=Debug
            ;;
        --dev|--local)
            ## Set good options for a local build
            export ENABLE_STRIP=0
            CMAKE_BUILD_TYPE=Debug
            ## gcc 7 no longer supported
            LOCAL_GCC=0
            LOCAL_CMAKE=1
            #NPROC=4
            CLEAN=0
            UNITTEST=1
            BUILD_SDDS3=0
            ;;
        --no-sdds3)
            BUILD_SDDS3=0
            ;;
        --sdds3)
            BUILD_SDDS3=1
            ;;
        --centos7-local|--centos7|--centos)
            export ENABLE_STRIP=0
            CMAKE_BUILD_TYPE=Debug
            LOCAL_GCC=0
            LOCAL_CMAKE=0
            NPROC=1
            CLEAN=0
            UNITTEST=1
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
        --local-gcc|--no-gcc)
            LOCAL_GCC=1
            ;;
        --local-cmake|--no-cmake)
            LOCAL_CMAKE=1
            ;;
        --no-unpack|--nounpack)
            NO_UNPACK=1
            ;;
        --dump-last-test-on-failure)
            DUMP_LAST_TEST_ON_FAILURE=1
            ;;
        --cmake-option)
            shift
            EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} $1"
            ;;
        --plugin-api)
            shift
            EXTRA_CMAKE_OPTIONS="${EXTRA_CMAKE_OPTIONS} -DPLUGINAPIPATH=$1"
            ;;
        --fuzz)
            export USE_LIBFUZZER=1
            CMAKE_BUILD_TYPE=Debug
            UNITTEST=0
            LOCAL_CMAKE=0
            BUILD_SDDS3=0
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
        --clean-log)
            : >$LOG
            ;;
        --unit-test|--unittest|--test)
            UNITTEST=1
            ;;
        --no-unit-test|--no-test|--no-unittest|--no-unittests)
            UNITTEST=0
            ;;
        --esg-ci-coverage)
            BULLSEYE=1
            UNITTEST=1
            BULLSEYE_UPLOAD=0
            COV_HTML_BASE=${COV_HTML_BASE%-dev}
            SDDS3=0
            ;;
        --bullseye|--bulleye)
            BULLSEYE=1
            BULLSEYE_UPLOAD=0
            UNITTEST=1
            SDDS3=0
            ;;
        --cpp-check)
            RUN_CPPCHECK=1
            NO_BUILD=1
            ;;
        --bullseye-upload-unittest|--bullseye-upload)
            BULLSEYE_UPLOAD=1
            ;;
        --valgrind)
            VALGRIND=1
            ;;
        --tap)
            shift
            TAP=$1
            ;;
        --redist)
            shift
            REDIST=$1
            ;;
        --build-dir)
            shift
            BUILD_DIR=$1
            ;;
        --get-input|--get-input-new)
            rm -rf input redist
            export TAP_PARAMETER_MODE=release
            $TAP fetch av_plugin.build.normal_build
            ;;
        --setup)
            rm -rf input redist
            export TAP_PARAMETER_MODE=release
            $TAP fetch av_plugin.build.normal_build
            NO_BUILD=1
            LOCAL_GCC=1
            LOCAL_CMAKE=1
            ;;
         --999)
            export VERSION_OVERRIDE=9.99.9.999
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

[[ -n "$BUILD_DIR" ]] || BUILD_DIR=build64-${CMAKE_BUILD_TYPE}
[[ -n "$REDIST" ]] || REDIST=$BASE/redist

[[ -n "${PLUGIN_NAME}" ]] || PLUGIN_NAME=${DEFAULT_PRODUCT}
[[ -n "${PRODUCT}" ]] || PRODUCT=${PLUGIN_NAME}

PRODUCT_UC=$(echo $PRODUCT | tr 'a-z' 'A-Z')

[[ -n "${PRODUCT_NAME}" ]] || PRODUCT_NAME="SPL-Anti-Virus-Plugin"
[[ -n "${PRODUCT_LINE_ID}" ]] || PRODUCT_LINE_ID="ServerProtectionLinux-Plugin-${PRODUCT_UC}"
[[ -n "${DEFAULT_HOME_FOLDER}" ]] || DEFAULT_HOME_FOLDER="$PRODUCT"

export NO_REMOVE_GCC=1

INPUT=$BASE/input
if [[ ! -d "$INPUT" ]]
then
    MESSAGE_PART1="You need to run the following to setup your input folder: "
    MESSAGE_PART2="./build.sh --setup"
    exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
fi

function untar_input()
{
    local input=$1
    local tarbase=$2
    local override_tar=$3
    local optional=$4
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
    elif [[ -z $optional ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

function unzip_lrdata()
{
    local LRDATA_ZIP=${INPUT}/lrdata/reputation.zip
    local DEST="${REDIST}/lrdata"
    rm -rf "${DEST}"
    mkdir -p "$DEST"

    if [[ ! -f "$LRDATA_ZIP" ]]
    then
        echo "Can't find $LRDATA_ZIP - expected if we have a production build"
        return
    fi

    pushd "$DEST"
    unzip "$LRDATA_ZIP"
    popd
}

function setup_susi()
{
    python3 $BASE/build/setup_tree.py
}

function cppcheck_build()
{
    local CPP_XML_REPORT="err.xml"
    local CPP_REPORT_DIR="cppcheck"
    local CPPCHECK=${CPPCHECK:-cppcheck}
    mkdir -p ${CPP_REPORT_DIR}
    ${CPPCHECK} --inline-suppr --xml --quiet --force \
        --template="[{severity}][{id}] {message} {callstack} \(On {file}:{line}\)" \
        -i tests/googletest/ \
        -I modules \
        modules products tests 2> ${CPP_REPORT_DIR}/${CPP_XML_REPORT}
    [[ -f ${CPP_REPORT_DIR}/${CPP_XML_REPORT} ]] || exitFailure $FAILURE_CPPCHECK "cppcheck failed to create report"
    python3 "$BASE/build/analysis/cpp_check_html_report.py" --file=${CPP_REPORT_DIR}/${CPP_XML_REPORT} --report-dir=${CPP_REPORT_DIR} --source-dir=${BASE}
    local ANALYSIS_ERRORS=$(grep 'severity="error"' ${CPP_REPORT_DIR}/${CPP_XML_REPORT} | wc -l)
    local ANALYSIS_WARNINGS=$(grep 'severity="warning"' ${CPP_REPORT_DIR}/${CPP_XML_REPORT} | wc -l)
    local ANALYSIS_PERFORMANCE=$(grep 'severity="performance"' ${CPP_REPORT_DIR}/${CPP_XML_REPORT} | wc -l)
    local ANALYSIS_INFORMATION=$(grep 'severity="information"' ${CPP_REPORT_DIR}/${CPP_XML_REPORT} | wc -l)
    local ANALYSIS_STYLE=$(grep 'severity="style"' ${CPP_REPORT_DIR}/${CPP_XML_REPORT} | wc -l)

    echo "The full XML static analysis report:"
    cat ${CPP_REPORT_DIR}/${CPP_XML_REPORT}
    echo "There are $ANALYSIS_ERRORS static analysis error issues"
    echo "There are $ANALYSIS_WARNINGS static analysis warning issues"
    echo "There are $ANALYSIS_PERFORMANCE static analysis performance issues"
    echo "There are $ANALYSIS_INFORMATION static analysis information issues"
    echo "There are $ANALYSIS_STYLE static analysis style issues"

    local ANALYSIS_OUTPUT_DIR="${OUTPUT}/analysis/"
    [[ -d ${ANALYSIS_OUTPUT_DIR} ]] || mkdir -p "${ANALYSIS_OUTPUT_DIR}"
    cp -a ${CPP_REPORT_DIR}  "${ANALYSIS_OUTPUT_DIR}" || exitFailure $FAILURE_COPY_CPPCHECK_RESULT_FAILED  "Failed to copy cppcheck report to output"

     # Fail the build if there are any static analysis warnings or errors.
    [[ $ANALYSIS_ERRORS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_ERRORS static analysis errors"
    [[ $ANALYSIS_WARNINGS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_WARNINGS static analysis warnings"
}

function build()
{
    local BITS=64

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    if [[ ! -d $INPUT ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi

    if [[ -z "$NO_UNPACK" ]]
    then
        rm -rf "$REDIST"
        mkdir -p "$REDIST"
        setup_susi
        (( LOCAL_GCC == 0 )) && unpack_scaffold_gcc_make "$INPUT"
        untar_input pluginapi "" "${PLUGIN_TAR}"
        python3 ${BASE}/build-files/create_library_links.py ${REDIST}/pluginapi
        (( LOCAL_CMAKE == 0 )) && ln -snf $INPUT/cmake $REDIST/
        untar_input capnproto
        untar_input boost
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
        untar_input openssl
    else
        (( LOCAL_GCC == 0 )) && set_gcc_make
    fi

    if (( LOCAL_CMAKE == 0 ))
    then
      addpath "$REDIST/cmake/bin"
      chmod 700 $REDIST/cmake/bin/cmake || exitFailure "Unable to chmod cmake"
      chmod 700 $REDIST/cmake/bin/ctest || exitFailure "Unable to chmod ctest"
    fi

    cp -r $REDIST/googletest $BASE/tests

    [[ -e ${REDIST}/sdds3 ]] || ln -s ../input/sdds3 "${REDIST}/sdds3" && chmod +x ${REDIST}/sdds3/*

    if (( RUN_CPPCHECK == 1 ))
    then
      cppcheck_build || exitFailure $FAILURE_CPPCHECK "Cppcheck static analysis build failed: $?"
    fi

    if (( NO_BUILD == 1 ))
    then
        return 0
    fi

    [[ -n ${NPROC:-} ]] || NPROC=$(nproc || echo 2)

    if (( BULLSEYE == 1 ))
    then
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bullseye"
        addpath ${BULLSEYE_DIR}/bin
        export LD_LIBRARY_PATH=${BULLSEYE_DIR}/lib:${LD_LIBRARY_PATH}
        export COVFILE
        export COVSRCDIR=$PWD
        export COV_HTML_BASE
        export BULLSEYE_DIR
        [[ $CLEAN == 1 ]] && rm -f "$COVFILE"
        bash -x "$BASE/build/bullseye/createCovFile.sh" \
            || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
        covclear || exitFailure $FAILURE_BULLSEYE "Unable to clear results"
    fi

    #   Required for build scripts to run on dev machines
    export LIBRARY_PATH=/build/input/gcc/lib64:/usr/lib/x86_64-linux-gnu${LIBRARY_PATH:+:${LIBRARY_PATH}}
    export CPLUS_INCLUDE_PATH=/build/input/gcc/include:/usr/include/x86_64-linux-gnu${CPLUS_INCLUDE_PATH:+:${CPLUS_INCLUDE_PATH}}
    export CPATH=/build/input/gcc/include:/usr/include/x86_64-linux-gnu${CPATH:+:${CPATH}}
    echo "After setup: LIBRARY_PATH=${LIBRARY_PATH}"

    export LD_LIBRARY_PATH=${LIBRARY_PATH}:$(pwd)/${BUILD_DIR}/libs:${BUILD_DIR}/libs
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"

    if [[ ! $USE_LIBFUZZER ]]
    then
      [[ -n $CXX ]] || CXX=$(which g++)
      [[ -n $CC ]] || CC=$(which gcc)
    else
      CXX=$(which clang++-9)
      CC=$(which clang-9)
    fi

    export CC
    export CXX

    rm -rf output
    mkdir -p output

    [[ $CLEAN == 1 ]] && rm -rf ${BUILD_DIR}
    mkdir -p ${BUILD_DIR}
    cd ${BUILD_DIR}
    which cmake
    LD_LIBRARY_PATH=${LD_LIBRARY_PATH} \
        cmake \
            -DINPUT="${REDIST}" \
            -DREDIST="${REDIST}" \
            -DPLUGIN_NAME="${PLUGIN_NAME}" \
            -DPRODUCT_NAME="${PRODUCT_NAME}" \
            -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
            -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
            -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
            -DCMAKE_CXX_COMPILER=$CXX \
            -DCMAKE_C_COMPILER=$CC \
            ${EXTRA_CMAKE_OPTIONS} \
        .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} "CXX=$CXX" "CC=$CC" "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}" \
        || exitFailure 15 "Failed to build $PRODUCT"

    if (( BULLSEYE == 1 ))
    then
      # copy empty covfile into output
      cp -a ${COVFILE} ${OUTPUT}/  \
          || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
    fi

    if (( VALGRIND == 1 ))
    then
        ## -VV --debug
        ctest \
          --test-action memcheck \
          --parallel ${NPROC} \
          --output-on-failure \
         || {
            local EXITCODE=$?
            exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
        }
    elif (( UNITTEST == 1 ))
    then
        ctest \
          --parallel ${NPROC} \
          --timeout 15 \
          --output-on-failure \
        || {
            local EXITCODE=$?
            echo "Unit tests failed with $EXITCODE"
            if (( DUMP_LAST_TEST_ON_FAILURE == 1 ))
            then
                cat Testing/Temporary/LastTest.log || true
            fi
            cp Testing/Temporary/LastTest.log $BASE/log/ || true
            exitFailure $FAILURE_UNIT_TESTS "Unit tests failed for $PRODUCT"
        }
    fi
    make install CXX=$CXX CC=$CC || exitFailure 17 "Failed to install $PRODUCT"
    make dist_sdds2 CXX=$CXX CC=$CC ||  exitFailure $FAILURE_DIST_FAILED "Failed to create dist $PRODUCT"
    if (( BUILD_SDDS3 == 1 ))
    then
        make dist_sdds3 CXX=$CXX CC=$CC || exitFailure $FAILURE_DIST_FAILED "Failed to create dist $PRODUCT"
    fi
    make sdds CXX=$CXX CC=$CC ||  exitFailure $FAILURE_DIST_FAILED "Failed to create sdds component $PRODUCT"
    cd ..

    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    local INSTALLSET=${BUILD_DIR}/installset
    local SDDS=${BUILD_DIR}/sdds
    local SDDS3=${BUILD_DIR}/SDDS3-PACKAGE

    if [[ -d $SDDS ]]
    then
        echo "Separate SDDS component"
        [[ -f $SDDS/SDDS-Import.xml ]] || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to create SDDS-Import.xml"
        cp -rL "$SDDS" output/SDDS-COMPONENT || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy Plugin SDDS component to output"
        if (( BUILD_SDDS3 == 1 ))
        then
            cp -rL "$SDDS3" output/SDDS3-PACKAGE || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy Plugin SDDS3 package to output"
            cp -rL $SDDS/SDDS-Import.xml output/SDDS3-PACKAGE || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS-Import.xml to SDDS3 package output"
        fi
    else
        exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to find SDDS component in build"
    fi
    if [[ -d "${INPUT}/base-sdds" ]]
    then
        cp -rL "${INPUT}/base-sdds"  output/base-sdds \
            || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy SSPL-Base SDDS component to output"
        chmod 755 output/base-sdds/{install.sh,files/bin/*,files/base/bin/*}
    fi
    if [[ -d ${BUILD_DIR}/componenttests ]]
    then
        cp -rL ${BUILD_DIR}/componenttests output/componenttests \
            || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy google component tests"
    fi
    if [[ -x ${BUILD_DIR}/tools/avscanner/mountinfoimpl/PrintMounts ]]
    then
        mkdir -p output/componenttests
        cp -rL ${BUILD_DIR}/tools/avscanner/mountinfoimpl/PrintMounts  output/componenttests/ \
            || exitFailure $FAILURE_COPY_SDDS_FAILED  "Failed to copy PrintMounts"
    fi
    mkdir -p output/manualtests
    cp -L TA/manual/*.sh TA/manual/*.py output/manualtests/

    python3 TA/process_capnp_files.py

    if [[ -d ${BUILD_DIR}/symbols ]]
    then
        cp -a ${BUILD_DIR}/symbols output/
    fi


    if (( BULLSEYE == 1 ))
    then
      cp -a ${COVFILE} ${OUTPUT}/${COVFILE_UNIT} \
          || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"

      export BASE
      export htmldir=${OUTPUT}/coverage_html
      cd "$BASE"

      bash -x build/bullseye/generateResults.sh || exit $?

      # copy the source into output, so that tests can generate detailed coverage reports
      mkdir -p ${OUTPUT}/src
      cp -a modules products ${OUTPUT}/src/

      if (( BULLSEYE_UPLOAD == 1 ))
      then
          ## Process bullseye output
          ## upload unit tests
          UPLOAD_ONLY=1 bash -x build/bullseye/uploadResults.sh || exit $?
      fi
    fi

    echo "Build Successful"
    return 0
}

build 2>&1 | tee $LOG
EXIT=$?
mkdir -p $OUTPUT
cp $LOG $OUTPUT/ || true
exit $EXIT
