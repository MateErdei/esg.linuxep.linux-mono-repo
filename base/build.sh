#!/bin/bash

PRODUCT=sspl-base
FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53
BUILD_TOOLS_NOT_SETUP=54
FAILURE_COPY_CPPCHECK_RESULT_FAILED=61
FAILURE_CPPCHECK=62

set -e
#set -x
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output
export BASE
export OUTPUT


function source_file()
{
  local path="$1"
  [ -f "$path" ] || { echo "Can't find $path" ; exit 11 ; }
  source "$path"
}

source_file "$BASE/build/common.sh"
source_file "$BASE/setup_env_vars.sh"

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

RELEASE_BUILD_TYPE="RelWithDebInfo"
DEBUG_BUILD_TYPE="Debug"
PythonCoverage="OFF"
STRACE_SUPPORT="OFF"
DISABLE_SANITIZER="OFF"
[[ -n "$CLEAN" ]] || CLEAN=0
BULLSEYE=0
BULLSEYE_UPLOAD=0
COVFILE="/tmp/root/sspl-base-unittest.cov"

CMAKE_BUILD_TYPE=$DEBUG_BUILD_TYPE

export ENABLE_STRIP=1
VALGRIND=0
VAGRANT=0
UNIT_TESTS=1
export SDDS3=1

# Deal with arguments
while [[ $# -ge 1 ]]
do
    case $1 in
        --dev)
            CLEAN=0
            UNIT_TESTS=1
            CMAKE_BUILD_TYPE=$DEBUG_BUILD_TYPE
            export ENABLE_STRIP=0
            unset SDDS3
            ;;
        --clean)
            CLEAN=1
            ;;
        --no-clean|--noclean)
            CLEAN=0
            ;;
        --input)
            shift
            INPUT=$1
            ;;
        --debug)
            CMAKE_BUILD_TYPE=$DEBUG_BUILD_TYPE
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
            CMAKE_BUILD_TYPE=$RELEASE_BUILD_TYPE
            export ENABLE_STRIP=1
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
        --bullseye|--coverage)
            BULLSEYE=1
            ;;
        --covfile)
            shift
            COVFILE=$1
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
        --disable-sanitizer|--no-sanitizer|--no-asan)
            DISABLE_SANITIZER="ON"
            ;;
        --fetch|--setup)
            "$BASE/tap_fetch.sh"
            ;;
        --setup-venv)
            rm -rf "${BASE}/tap_venv"
            PYTHON="${PYTHON:-python3.7}"
            [[ -x "$(which $PYTHON)" ]] || PYTHON=python3
            "${PYTHON}" -m venv "${BASE}/tap_venv"
            source "$BASE/tap_venv/bin/activate"
            cat <<EOF >"${BASE}/tap_venv/pip.conf"
[global]
timeout=60
index-url = https://artifactory.sophos-ops.com/api/pypi/pypi/simple
EOF
            pip install --upgrade pip
            pip install --upgrade wheel build_scripts
            pip install --upgrade tap keyrings.alt
            exit 0
            ;;
        --skip-sdds3|--no-sdds3)
          unset SDDS3
          ;;
        --sdds3)
          export SDDS3=1
          ;;
        --vagrant)
          VAGRANT=1
          ;;
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done


# Handle detecting if we're doing a local build or a CI build
if [[ "$CI" == "true" ]]
then
  echo "Detected that this is a CI build"
  CLEAN=1
  TEST_NPROC=1
  set -x
else
  echo "Detected that this is a non-CI (local) build"
fi

# Handle the case where someone may have wiped redist and inputs dir and not re-run the setup script.
if [[ ! -x "${CXX}" ]]
then
    exitFailure $BUILD_TOOLS_NOT_SETUP "Compiler not found CXX:[$CXX]. $CXX should be setup by running setup_build_tools.sh"
fi


function cppcheck_build()
{
    local build_dir="$1"
    [[ -d "$build_dir" ]] || mkdir -p $"build_dir"
    CURR_WD=$(pwd)
    cd ${build_dir}
    cmake "$BASE"
    CPP_XML_REPORT="err.xml"
    CPP_REPORT_DIR="cppcheck"
    cppcheck --version
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
    echo "Static analysis results: $ANALYSIS_OUTPUT_DIR/cppcheck/index.html"
    [[ -d ${ANALYSIS_OUTPUT_DIR} ]] || mkdir -p "${ANALYSIS_OUTPUT_DIR}"
    cp -a ${CPP_REPORT_DIR} "${ANALYSIS_OUTPUT_DIR}" || exitFailure $FAILURE_COPY_CPPCHECK_RESULT_FAILED "Failed to copy cppcheck report to output"
    cd "${CURR_WD}"

    # Fail the build if there are any static analysis warnings or errors.
    [[ $ANALYSIS_ERRORS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_ERRORS static analysis errors"
    [[ $ANALYSIS_WARNINGS == 0 ]] || exitFailure $FAILURE_CPPCHECK "Build failed. There are $ANALYSIS_WARNINGS static analysis warnings"
}

function build()
{
    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"

    echo "Building with env:"
    env
    echo "-----------------"

    if [[ ! -d "$FETCHED_INPUTS_DIR" ]]
    then
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No input available"
    fi

    if [[ $CLEAN == 1 ]]
    then
      rm -rf "$BASE/$DEBUG_BUILD_DIR"
      rm -rf "$BASE/$RELEASE_BUILD_DIR"
      rm -rf "$OUTPUT"
      rm -f "$LOG"
    fi

    if [[ "$CMAKE_BUILD_TYPE" == "$RELEASE_BUILD_TYPE" ]]
    then
      echo "Build type is release"
      BUILD_DIR="$BASE/$RELEASE_BUILD_DIR"
    elif [[ "$CMAKE_BUILD_TYPE" == "$DEBUG_BUILD_TYPE" ]]
    then
      echo "Build type is debug"
      BUILD_DIR="$BASE/$DEBUG_BUILD_DIR"
    else
      # https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
      exitFailure $FAILURE_BAD_ARGUMENT "CMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE but should be either $DEBUG_BUILD_TYPE or $RELEASE_BUILD_TYPE"
    fi

    # Run static analysis, fails build if it finds any warnings or errors.
    if [[ $ANALYSIS == 1 ]]
    then
      cppcheck_build  $BUILD_DIR || exitFailure $FAILURE_CPPCHECK "Cppcheck static analysis build failed: $?"
    fi

    if [[ "$NO_BUILD" == "1" ]]
    then
        echo "Not building (NO_BUILD=1)"
        exit 0
    fi

    COVERAGE_DEF=""
    if [[ ${BULLSEYE} == 1 ]]
    then
        COVERAGE_DEF="-DCOVERAGE_DEF=1"
        BULLSEYE_DIR=/opt/BullseyeCoverage
        [[ -d $BULLSEYE_DIR ]] || BULLSEYE_DIR=/usr/local/bullseye
        [[ -d $BULLSEYE_DIR ]] || exitFailure $FAILURE_BULLSEYE "Failed to find bullseye"
        export PATH=${PATH}:${BULLSEYE_DIR}/bin:$PATH
        export LD_LIBRARY_PATH=${BULLSEYE_DIR}/lib:${LD_LIBRARY_PATH}
        export COVFILE
        export BULLSEYE_DIR
        bash -x "$BASE/build/bullseye/createCovFile.sh" || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
        covclear || exitFailure $FAILURE_BULLSEYE "Unable to clear results"
    fi


    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    [[ -n ${NPROC:-} ]] || { which nproc > /dev/null 2>&1 && NPROC=$((`nproc`)); } || NPROC=2
    (( $NPROC < 1 )) && NPROC=1

    # Some dev laptops have 16 cores but only 32GB RAM (16GB for WSL) and 15 build threads will trigger OOM
    (( $NPROC > 8 )) && NPROC=7

    cmake -G "Unix Makefiles" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
          -DNO_GCOV="true" \
          -DPythonCoverage="${PythonCoverage}" \
          -DSTRACE_SUPPORT="$STRACE_SUPPORT" \
          -DDISABLE_SANITIZER="$DISABLE_SANITIZER" \
          $COVERAGE_DEF \
          "$BASE" \
          || exitFailure 14 "Failed to configure $PRODUCT"

    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"

    if [[ "$UNIT_TESTS" == "1" ]]
    then
        if (( ${VALGRIND} == 1 ))
        then
            ## -VV --debug
            export NPROC
            bash ${BASE}/build/valgrind/runValgrind.sh "$BUILD_DIR" \
             || {
                local EXITCODE=$?
                exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
            }
            echo 'Valgrind test finished'
            exit 0
        fi
        [[ -n ${TEST_NPROC:-} ]] || TEST_NPROC=$NPROC
        timeout 2010s ctest \
            --parallel ${TEST_NPROC} \
            --test-action test \
            --no-compress-output --output-on-failure \
            --timeout 300 \
            || {
              local EXITCODE=$?
              echo "Unit tests failed with $EXITCODE"
              if [[ "$EXITCODE" == "124" ]]
              then
                  echo "Unit tests timed out"
              fi
              cat Testing/Temporary/LastTest.log || true
              cat /tmp/unitTest.log || true
              exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
            }

        cat /tmp/unitTest.log || true

        # python would have been executed in unit tests creating pyc and or pyo files
        # need to make sure these are removed before creating distribution files.
        find "$BUILD_DIR" -name "*.pyc" -type f | xargs -r rm -f
        find "$BUILD_DIR" -name "*.pyo" -type f | xargs -r rm -f
    fi
    make -j${NPROC} install || exitFailure 17 "Failed to install $PRODUCT"

    echo "output contents:"
    ls -l "$OUTPUT"

    if [[ ${BULLSEYE} == 1 ]]
    then
      ## upload unit tests
      if [[ ${UNIT_TESTS} == 1 ]]
      then
            ## Process bullseye output
            ## upload unit tests
            cd "$BASE"

            #keep the local jenkins tests seperated
            export COV_HTML_BASE=sspl-base-unittest
            export BULLSEYE_UPLOAD
            bash -x build/bullseye/uploadResults.sh || exit $?
      fi
      cp -a ${COVFILE}  output   || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to copy covfile: $?"
    fi

    if (( VAGRANT == 1 ))
    then
        pwd
        cd ..
        ./vagrant rsync
    fi

    echo "Build completed"
}

build 2>&1 | tee -a "$LOG"
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
