#!/bin/bash
PRODUCT=sspl-base

source /etc/profile
set -ex
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

export NO_REMOVE_GCC=1

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
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

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

    local INPUT=$BASE/input
    local ALLEGRO_REDIST=/redist/binaries/linux11/input

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

        local VERSIG_TAR=$INPUT/versig.tar
        if [[ -f $VERSIG_TAR ]]
        then
            tar xf "$VERSIG_TAR" -C "$REDIST"
        else
            ln -snf $ALLEGRO_REDIST/versig $REDIST/versig
        fi

        local CURL_TAR=$INPUT/curl.tar
        if [[ -f "$CURL_TAR" ]]
        then
            tar xf "$CURL_TAR" -C "$REDIST"
        else
            ln -snf $ALLEGRO_REDIST/curl $REDIST/curl
        fi

        local SUL_TAR=$INPUT/SUL.tar
        if [[ -f "$SUL_TAR" ]]
        then
            tar xf "$SUL_TAR" -C "$REDIST"
        else
            ln -snf $ALLEGRO_REDIST/SUL $REDIST/SUL
        fi

        local BOOST_TAR=$INPUT/boost.tar
        if [[ -f "$BOOST_TAR" ]]
        then
            tar xf "$BOOST_TAR" -C "$REDIST"
        else
            ln -snf $ALLEGRO_REDIST/boost $REDIST/boost
        fi

        local EXPAT_TAR=$INPUT/expat.tar
        if [[ -f "$EXPAT_TAR" ]]
        then
            tar xf "$EXPAT_TAR" -C "$REDIST"
        elif [[ -d $ALLEGRO_REDIST ]]
        then
            ln -snf $ALLEGRO_REDIST/expat $REDIST/expat
        else
            exitFailure 13 "Failed to find expat"
        fi

        ## Google protobuf
        local PROTOBUF_TAR=$INPUT/protobuf.tar
        if [[ -f "$PROTOBUF_TAR" ]]
        then
            tar xf "$PROTOBUF_TAR" -C "$REDIST"
        elif [[ -d $ALLEGRO_REDIST ]]
        then
            ln -snf $ALLEGRO_REDIST/protobuf $REDIST/protobuf
        else
            exitFailure 13 "Failed to find protobuf"
        fi
        addpath ${REDIST}/protobuf/install${BITS}/bin
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/protobuf/install${BITS}/lib

        ## ZeroMQ
        local ZEROMQ_TAR=$INPUT/zeromq.tar
        if [[ -f "$ZEROMQ_TAR" ]]
        then
            tar xf "$ZEROMQ_TAR" -C "$REDIST"
        elif [[ -d $ALLEGRO_REDIST ]]
        then
            ln -snf $ALLEGRO_REDIST/zeromq $REDIST/zeromq
        else
            exitFailure 13 "Failed to find zeromq"
        fi
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/zeromq/lib

        local LOG4CPLUS_TAR=$INPUT/log4cplus.tar
        if [[ -f "$LOG4CPLUS_TAR" ]]
        then
            tar xf "$LOG4CPLUS_TAR" -C "$REDIST"
        elif [[ -d "$ALLEGRO_REDIST" ]]
        then
            ln -snf $ALLEGRO_REDIST/log4cplus $REDIST/log4cplus
        else
            exitFailure 13 "Failed to find log4cplus"
        fi
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/log4cplus/lib
    else
        echo "WARNING: No input available; using system or /redist files"
        REDIST=$ALLEGRO_REDIST
    fi

    ZIP=$(which zip 2>/dev/null || true)
    [[ -x "$ZIP" ]] || {
        echo "Installing zip"
        sudo yum install -y zip unzip </dev/null
    }

    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"


    mkdir -p build${BITS}
    cd build${BITS}
    [[ -n ${NPROC:-} ]] || NPROC=2
    cmake -v -DREDIST="${REDIST}" -DINPUT="${REDIST}" .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"
    make CTEST_OUTPUT_ON_FAILURE=1 test || {
        cat Testing/Temporary/LastTest.log || true
        exitFailure 16 "Unit tests failed for $PRODUCT: $?"
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
    echo "Build completed"
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
