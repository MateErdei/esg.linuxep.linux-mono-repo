#!/bin/bash
PRODUCT=sspl-base

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

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

CLEAN=0
BULLSEYE=0
BULLSEYE_UPLOAD_UNITTEST=0
export NO_REMOVE_GCC=1
ALLEGRO_REDIST=/redist/binaries/linux11/input
INPUT=$BASE/input
COVFILE="/tmp/root/sspl.cov"

while [[ $# -ge 1 ]]
do
    case $1 in
        --clean)
            CLEAN=1
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
        --bullseye|--bulleye)
            BULLSEYE=1
            ;;
        --covfile)
            shift
            COVFILE=$1
            ;;
        --bullseye-upload-unittest)
            BULLSEYE_UPLOAD_UNITTEST=1
            ;;
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done

function untar_or_link_to_redist()
{
    local input=$1
    local tar=${INPUT}/${input}.tar

    if [[ -f "$tar" ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
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

    ## These can't be exitFailure since it doesn't exist till the sourcing is done
    [ -f "$BASE"/build/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
    source "$BASE"/build/pathmgr.sh
    [ -f "$BASE"/build/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
    source "$BASE"/build/common.sh

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "Initial PATH=$PATH"
    echo "Initial LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    cd $BASE
    ## Need to do this before we set LD_LIBRARY_PATH, since it uses ssh
    ## which doesn't like our openssl
    git submodule sync --recursive || exitFailure 34 "Failed to sync submodule configuration"
    GIT_SSL_NO_VERIFY=true  \
        git -c http.sslVerify=false submodule update --init --recursive || {
        sleep 1
        echo ".gitmodules:"
        cat .gitmodules
        echo ".git/config:"
        cat .git/config
        exitFailure 33 "Failed to get googletest via git"
    }

    if [[ -d $INPUT ]]
    then

        unpack_scaffold_gcc_make "$INPUT"

        REDIST=$BASE/redist
        mkdir -p $REDIST

        OPENSSL_TAR=${INPUT}/openssl.tar
        if [[ -f $OPENSSL_TAR ]]
        then
            rm -rf $REDIST/openssl
            tar xf "$OPENSSL_TAR" -C "$REDIST"
            ln -snf libssl.so.1 ${REDIST}/openssl/lib${BITS}/libssl.so.10
            ln -snf libcrypto.so.1 ${REDIST}/openssl/lib${BITS}/libcrypto.so.10
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
        export BULLSEYE_DIR
        bash -x "$BASE/build/bullseye/createCovFile.sh" || exitFailure $FAILURE_BULLSEYE_FAILED_TO_CREATE_COVFILE "Failed to create covfile: $?"
        export CC=$BULLSEYE_DIR/bin/gcc
        export CXX=$BULLSEYE_DIR/bin/g++
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

    [[ -n ${NPROC:-} ]] || NPROC=2
    cmake -v -DREDIST="${REDIST}" -DINPUT="${REDIST}" .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"
    make CTEST_OUTPUT_ON_FAILURE=1 test || {
        local EXITCODE=$?
        echo "Unit tests failed with $EXITCODE"
        cat Testing/Temporary/LastTest.log || true
        exitFailure 16 "Unit tests failed for $PRODUCT: $EXITCODE"
    }
    make install || exitFailure 17 "Failed to install $PRODUCT"
    make dist || exitFailure 18 "Failed to create distribution"
    cd ..

    mkdir -p output
    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    rm -rf output/SDDS-COMPONENT
    cp -a build${BITS}/distribution/ output/SDDS-COMPONENT || exitFailure 21 "Failed to copy SDDS package: $?"
    cp -a build${BITS}/modules/Common/PluginApiImpl/pluginapi.tar.gz output/pluginapi.tar.gz || exitFailure 22 "Failed to copy pluginapi.tar.gz package: $?"


    if [[ ${BULLSEYE_UPLOAD_UNITTEST} == 1 ]]
    then
        ## Process bullseye output
        bash build/bullseye/uploadResults.sh || exit $?
    fi

    echo "Build completed"
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
