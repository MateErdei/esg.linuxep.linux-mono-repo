#!/bin/bash
PRODUCT=versig

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
    [ -f "$BASE"/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
    source "$BASE"/pathmgr.sh
    [ -f "$BASE"/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
    source "$BASE"/common.sh

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"

    cd $BASE
    ## Need to do this before we set LD_LIBRARY_PATH, since it uses ssh
    ## which doesn't like our openssl
    git submodule update --init || {
        sleep 1
        echo ".gitmodules:"
        cat .gitmodules
        exitFailure 16 "Failed to get googletest via git"
    }

    unpack_scaffold_gcc_make $BASE/input

    OPENSSL_TAR=$BASE/input/openssl.tar
    [[ -f $OPENSSL_TAR ]] || exitFailure 12 "Failed to find openssl"

    REDIST=$BASE/redist

    mkdir -p $BASE/redist
    tar xf "$OPENSSL_TAR" -C "$REDIST"
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib${BITS}
    ln -snf libssl.so.1 ${REDIST}/openssl/lib${BITS}/libssl.so.10
    ln -snf libcrypto.so.1 ${REDIST}/openssl/lib${BITS}/libcrypto.so.10

    CMAKE_TAR=$(ls $BASE/input/cmake-*.tar.gz)
    [[ -f $CMAKE_TAR ]] || exitFailure 13 "Failed to find cmake"
    tar xzf "$CMAKE_TAR" -C "$REDIST"
    addpath "$REDIST/cmake/bin"

    COMMON_LDFLAGS="${LINK_OPTIONS}"
    COMMON_CFLAGS="${OPTIONS} ${CFLAGS} ${COMMON_LDFLAGS}"

    rm -rf ${PRODUCT}
    mkdir -p ${PRODUCT}

    ## build BITS
    mkdir -p ${PRODUCT}/${BITS}
    export CFLAGS="-m${BITS} ${COMMON_CFLAGS}"
    export LDFLAGS="-m${BITS} ${COMMON_LDFLAGS}"
    echo $CFLAGS >${PRODUCT}/${BITS}/CFLAGS
    echo $LDFLAGS >${PRODUCT}/${BITS}/LDFLAGS
    echo "$BITS: CFLAGS=$CFLAGS"
    echo "$BITS: LDFLAGS=$LDFLAGS"
    echo "STARTINGDIR=$STARTINGDIR" >${PRODUCT}/${BITS}/STARTINGDIR
    echo "BASE=$BASE" >${PRODUCT}/${BITS}/BASE
    echo "PATH=$PATH" >${PRODUCT}/${BITS}/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >${PRODUCT}/${BITS}/LD_LIBRARY_PATH

    rm -rf build${BITS}
    mkdir build${BITS}
    cd build${BITS}
    [[ -n $NPROC ]] || NPROC=2
    cmake -DREDIST="${REDIST}" .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"
    cd ..

    ## package output
    mkdir -p ${PRODUCT}/bin${BITS}
    cp build${BITS}/versig ${PRODUCT}/bin${BITS}

    ## DEBUG:
    echo "STARTINGDIR=$STARTINGDIR" >${PRODUCT}/STARTINGDIR
    echo "BASE=$BASE" >${PRODUCT}/BASE
    echo "PATH=$PATH" >${PRODUCT}/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >${PRODUCT}/LD_LIBRARY_PATH

    mkdir -p output
    tar cf output/${PRODUCT}.tar ${PRODUCT} || exitFailure 21 "Unable to pack result tarfile: $?"

    rm -rf ${PRODUCT}

    echo "Build completed"
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
