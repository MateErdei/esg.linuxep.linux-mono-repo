#!/bin/bash
PRODUCT=sspl-base
export PRODUCT_NAME="Sophos Server Protection Linux - Base"
export PRODUCT_LINE_ID="ServerProtectionLinux-Base"
export DEFAULT_HOME_FOLDER="sspl-base"

FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE=51
FAILURE_BULLSEYE=52
FAILURE_BAD_ARGUMENT=53

source /etc/profile
set -ex
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output
export BASE
export OUTPUT

## These can't be exitFailure since it doesn't exist till the sourcing is done
[ -f "$BASE"/build/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
source "$BASE"/build/pathmgr.sh
[ -f "$BASE"/build/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build/common.sh

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

CLEAN=0
BULLSEYE=0
BULLSEYE_UPLOAD=0
BULLSEYE_SYSTEM_TESTS=0
export NO_REMOVE_GCC=1
ALLEGRO_REDIST=/redist/binaries/linux11/input
INPUT=$BASE/input
COVFILE="/tmp/root/sspl-unit.cov"
COV_HTML_BASE=sspl-unittest
BULLSEYE_SYSTEM_TEST_BRANCH=master
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
        --allegro-redist)
            shift
            ALLEGRO_REDIST=$1
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
        --bullseye|--bulleye)
            BULLSEYE=1
            BULLSEYE_UPLOAD=1
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
            BULLSEYE_UPLOAD=1
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
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done

case ${CMAKE_BUILD_TYPE} in
    Debug|DEBUG)
        DEBUG=1
        export ENABLE_STRIP=0
        ;;
esac

function untar_or_link_to_redist()
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
    elif [[ -d "${ALLEGRO_REDIST}/$input" ]]
    then
        echo "Linking ${REDIST}/$input to ${ALLEGRO_REDIST}/$input"
        ln -snf "${ALLEGRO_REDIST}/$input" "${REDIST}/$input"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
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

    if [[ -d "$INPUT" ]]
    then

        unpack_scaffold_gcc_make "$INPUT"

        REDIST=$BASE/redist
        mkdir -p $REDIST

        OPENSSL_TAR=${INPUT}/openssl.tar
        if [[ -f $OPENSSL_TAR ]]
        then
            rm -rf $REDIST/openssl
            tar xf "$OPENSSL_TAR" -C "$REDIST"
        elif [[ -d $ALLEGRO_REDIST ]]
        then
            ln -snf $ALLEGRO_REDIST/openssl $REDIST/openssl
            echo "Failed to find input openssl, using allegro redist openssl"
        else
            exitFailure 12 "Failed to find openssl"
        fi
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib${BITS}

        local CMAKE_TAR=$(ls $INPUT/cmake-*.tar.gz)
        if [[ -f "$CMAKE_TAR" ]]
        then
            tar xzf "$CMAKE_TAR" -C "$REDIST"
            addpath "$REDIST/cmake/bin"
        else
            echo "WARNING: using system cmake"
        fi

        untar_or_link_to_redist versig
        untar_or_link_to_redist curl
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/curl/lib64
        untar_or_link_to_redist SUL
        untar_or_link_to_redist boost
        untar_or_link_to_redist expat
        untar_or_link_to_redist log4cplus
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/log4cplus/lib
        untar_or_link_to_redist zeromq
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/zeromq/lib
        untar_or_link_to_redist protobuf
        untar_or_link_to_redist capnproto
        untar_or_link_to_redist python
        untar_or_link_to_redist python-watchdog
        untar_or_link_to_redist python-pathtools
        untar_or_link_to_redist pycryptodome
        untar_or_link_to_redist $GOOGLETESTTAR
        addpath ${REDIST}/protobuf/install${BITS}/bin
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/protobuf/install${BITS}/lib

        mkdir -p ${REDIST}/certificates
        if [[ -f ${INPUT}/ps_certificates/ps_rootca.crt ]]
        then
            cp ${INPUT}/ps_certificates/ps_rootca.crt ${REDIST}/certificates
        elif [[ -f "$ALLEGRO_REDIST/certificates/ps_rootca.crt" ]]
        then
            cp "$ALLEGRO_REDIST/certificates/ps_rootca.crt" ${REDIST}/certificates
        else
            exitFailure $FAILURE_INPUT_NOT_AVAILABLE "ps_rootca.crt not available"
        fi

        if [[ -f ${INPUT}/manifest_certificates/rootca.crt ]]
        then
            cp "${INPUT}/manifest_certificates/rootca.crt" "${REDIST}/certificates/"
        elif [[ -f ${INPUT}/ps_certificates/ps_rootca.crt ]]
        then
            ## Use ps_rootca.crt as rootca.crt since they are the same in practice
            cp "${INPUT}/ps_certificates/ps_rootca.crt" "${REDIST}/certificates/rootca.crt"
        elif [[ -f "$ALLEGRO_REDIST/certificates/rootca.crt" ]]
        then
            cp "$ALLEGRO_REDIST/certificates/rootca.crt" "${REDIST}/certificates"
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

    elif [[ -d "$ALLEGRO_REDIST" ]]
    then
        echo "WARNING: No input available; using system or /redist files"
        REDIST=$ALLEGRO_REDIST
        addpath "$REDIST/cmake/bin"
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/curl/lib${BITS}
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib${BITS}
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/log4cplus/lib
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/zeromq/lib
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/protobuf/install${BITS}/lib
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/gcc/lib${BITS}
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No redist or input available"
    fi
    cp -r $REDIST/$GOOGLETESTTAR $BASE/tests/googletest
    ZIP=$(which zip 2>/dev/null || true)
    [[ -x "$ZIP" ]] || {
        echo "Installing zip"
        sudo yum install -y zip unzip </dev/null
    }

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
    fi

    echo "After setup: PATH=$PATH"
    echo "After setup: LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"


    [[ $CLEAN == 1 ]] && rm -rf build${BITS}
    mkdir -p build${BITS}
    cd build${BITS}
    echo "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >env
    echo "export PATH=$PATH" >>env

    [[ -n ${NPROC:-} ]] || { which nproc > /dev/null 2>&1 && NPROC=$(echo `nproc` / 2 | bc); } ||  NPROC=2
    [[ NPROC  -gt 0 ]] || NPROC=1
    cmake -v \
        -DPRODUCT_NAME="${PRODUCT_NAME}" \
        -DPRODUCT_LINE_ID="${PRODUCT_LINE_ID}" \
        -DDEFAULT_HOME_FOLDER="${DEFAULT_HOME_FOLDER}" \
        -DREDIST="${REDIST}" \
        -DINPUT="${REDIST}" \
        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
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
