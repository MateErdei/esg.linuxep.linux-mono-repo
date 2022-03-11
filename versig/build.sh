#!/bin/bash
PRODUCT=versig

source /etc/profile
set -ex
set -o pipefail

STARTINGDIR=$(pwd)

FAILURE_BAD_ARGUMENT=3
FAILURE_INPUT_NOT_AVAILABLE=4

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output
mkdir -p $OUTPUT

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1

## These can't be exitFailure since it doesn't exist until the sourcing is done
[ -f "$BASE"/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
source "$BASE"/pathmgr.sh
[ -f "$BASE"/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/common.sh

export NO_REMOVE_GCC=1

while [[ $# -ge 1 ]]
do
    case $1 in
        --no-build)
            NO_BUILD=1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

INPUT=/build/input
if [[ ! -d "$INPUT" ]]
then
    if [[ -d "$BASE/versig-build" ]]
    then
        INPUT="$BASE/versig-build/input"
    else
        MESSAGE_PART1="You need to run the following to setup your input folder: "
        MESSAGE_PART2="python3 -m build_scripts.artisan_fetch release-package.xml"
        exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
    fi
fi

function build()
{
    local BITS=$1

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    cd $BASE
    ## Need to do this before we set LD_LIBRARY_PATH, since it uses ssh
    ## which doesn't like our openssl
#    git submodule sync --recursive || exitFailure 34 "Failed to sync submodule configuration"
#    GIT_SSL_NO_VERIFY=true  \
#        git -c http.sslVerify=false submodule update --init --recursive || {
#        sleep 1
#        echo ".gitmodules:"
#        cat .gitmodules
#        echo ".git/config:"
#        cat .git/config
#        exitFailure 33 "Failed to get googletest via git"
#    }

    unpack_scaffold_gcc_make $INPUT

    rm -rf $BASE/tests/googletest
    cp -r $INPUT/googletest $BASE/tests

    OPENSSL_TAR=$INPUT/openssl.tar
    [[ -f $OPENSSL_TAR ]] || exitFailure 12 "Failed to find openssl"

    REDIST=$BASE/redist

    mkdir -p $BASE/redist
    tar xf "$OPENSSL_TAR" -C "$REDIST"
    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib${BITS}
    ln -snf libssl.so.1 ${REDIST}/openssl/lib${BITS}/libssl.so.10
    ln -snf libcrypto.so.1 ${REDIST}/openssl/lib${BITS}/libcrypto.so.10

    addpath "$INPUT/cmake/bin"
    chmod 700 $INPUT/cmake/bin/cmake || exitFailure "Unable to chmod cmake"

    COMMON_LDFLAGS="${LINK_OPTIONS:-}"
    COMMON_CFLAGS="${OPTIONS:-} ${CFLAGS:-} ${COMMON_LDFLAGS}"

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

    INSTALL=$BASE/$PRODUCT/${BITS}

    if [[ ${NO_BUILD} == 1 ]]
    then
        exit 0
    fi

    rm -rf build${BITS}
    mkdir build${BITS}
    cd build${BITS}
    [[ -n ${NPROC:-} ]] || NPROC=2
    cmake -DINPUT="${REDIST}" -DCMAKE_INSTALL_PREFIX=$INSTALL .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} || exitFailure 15 "Failed to build $PRODUCT"
    make test || exitFailure 16 "Unit tests failed for $PRODUCT"
    make install || exitFailure 17 "Failed to install $PRODUCT"
    cd ..

    ## package output
    mkdir -p ${PRODUCT}/bin${BITS}
    cp build${BITS}/versig ${PRODUCT}/bin${BITS}

    ## DEBUG:
    echo "STARTINGDIR=$STARTINGDIR" >${PRODUCT}/STARTINGDIR
    echo "BASE=$BASE" >${PRODUCT}/BASE
    echo "PATH=$PATH" >${PRODUCT}/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >${PRODUCT}/LD_LIBRARY_PATH

    tar cf output/${PRODUCT}.tar ${PRODUCT} || exitFailure 21 "Unable to pack result tarfile: $?"

    rm -rf ${PRODUCT}

    echo "Build completed"
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
