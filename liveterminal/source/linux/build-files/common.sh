##
## ALWAYS EDIT //version/product/savlinux/3rdparty/support/common.sh
##
function exitFailure()
{
    local CODE=$1
    shift
    echo "FAILURE - $*" | tee -a $LOG | tee -a /tmp/build.log
    exit $CODE
}

function failure()
{
    exitFailure 1 "$@"
}

function unpack_scaffold_gcc_make()
{
    local INPUT="$1"

    local GCC_TARFILE=$(ls $INPUT/gcc-*-$PLATFORM.tar.gz)
    if [[ -f $GCC_TARFILE ]]
    then
        if [[ -z "$NO_REMOVE_GCC" ]]
        then
            echo "Removing built-in gcc"
            sudo rpm -e gcc gcc-c++ || true
        fi

        mkdir -p /build/input
        pushd /build/input
        tar xzf $GCC_TARFILE
        popd

        export PATH=/build/input/gcc/bin:$PATH
        export LD_LIBRARY_PATH=/build/input/gcc/lib64:/build/input/gcc/lib32:$LD_LIBRARY_PATH
        export CC=/build/input/gcc/bin/gcc
        export CXX=/build/input/gcc/bin/g++
    elif [[ -d /build/input/gcc ]]
    then
        ## Already unpacked
        echo "WARNING: Using existing unpacked gcc"
        export PATH=/build/input/gcc/bin:$PATH
        export LD_LIBRARY_PATH=/build/input/gcc/lib64:/build/input/gcc/lib32:$LD_LIBRARY_PATH
        export CC=/build/input/gcc/bin/gcc
        export CXX=/build/input/gcc/bin/g++
    elif [[ -f /usr/local/bin/gcc ]]
    then
        ## Locally built gcc
        echo "WARNING: Using gcc from /usr/local/bin"
        export PATH=/usr/local/bin:$PATH
        export LD_LIBRARY_PATH=/usr/local/lib64:$LD_LIBRARY_PATH
        export CC=/usr/local/bin/gcc
        export CXX=/usr/local/bin/g++
    else
        echo "WARNING: Building with OS gcc/g++"
    fi
    which gcc
    GCC=$(which gcc)
    [[ -x $GCC ]] || exitFailure 50 "No gcc is available"

    local MAKE_TARFILE=$(ls $INPUT/make-*-$PLATFORM.tar.gz 2>/dev/null)
    if [[ -f $MAKE_TARFILE ]]
    then
        if [[ -z "$NO_REMOVE_MAKE" ]]
        then
            echo "Removing built-in make"
            sudo rpm -e make || true
        fi

        mkdir -p /build/input
        pushd /build/input
        tar xzf $MAKE_TARFILE
        popd

        export PATH=/build/input/make/bin:$PATH
    else
        echo "WARNING: Building with OS make"
    fi
    which make
    MAKE=$(which make)
    [[ -x $MAKE ]] || exitFailure 51 "No make is available"

    local BINUTILS_TARFILE=$(ls $INPUT/binutils*-$PLATFORM.tar.gz 2>/dev/null)
    if [[ -f $BINUTILS_TARFILE ]]
    then
        tar xzf $BINUTILS_TARFILE
        export PATH=/build/input/binutils/bin:$PATH
        export LD_LIBRARY_PATH=/build/input/binutils/lib:$LD_LIBRARY_PATH
    else
        echo "Warning: Building with OS binutils"
    fi
}


function unpack_scaffold_autotools()
{
    local INPUT="$1"

    mkdir -p /build/input
    pushd /build/input

    [[ -f $INPUT/autotools-$PLATFORM.tar.gz ]] || exitFailure 9 "No autotools tarfile"

    tar xzf $INPUT/autotools-$PLATFORM.tar.gz
    export PATH=/build/input/autotools/bin:$PATH

    popd
}

function unpack_scaffold_m4()
{
    local INPUT="$1"
    mkdir -p /build/input
    pushd /build/input

    local M4_TARFILE=$(ls $INPUT/m4-$PLATFORM.tar.gz)
    [[ -f $M4_TARFILE ]] || exitFailure 10 "No m4 tarfile $INPUT/m4-$PLATFORM.tar.gz"

    tar xzf $M4_TARFILE
    export PATH=/build/input/m4/bin:$PATH

    popd
    which m4
    [[ -x $(which m4) ]] || exitFailure 19 "No m4 available"
}

# Given libfoo.1.2.3 and a bunch of symlinks to it, as
# returned by ls, only return libfoo.1.2.3
function find_full_library_name()
{
    for LIBNAME in $1
    do
        if [[ ! -h $LIBNAME ]]
        then
            echo $LIBNAME
            return 0
        fi
    done

    echo "Could not find any non-symlinks!"
    exit 1
}

mkdir -p log
LOG=$BASE/log/build.log
date | tee -a $LOG | tee /tmp/build.log

unamestr=$(uname)
PLATFORM=`uname -s | LC_ALL=C tr '[:upper:]' '[:lower:]' | LC_ALL=C tr -d '-'`

# Ah HP-UX, always so excitingly different
if [[ "$unamestr" == "HP-UX" ]]; then
    cpustr=$(uname -m)
else
    cpustr=$(uname -p)
fi

BUILDARCH=$unamestr-$cpustr

echo "Build architecture is $BUILDARCH"

SECURITY_CPP="-D_FORTIFY_SOURCE=2"
SECURITY_COMPILE="-fstack-protector-all"
SECURITY_LINK="-Wl,-z,relro,-z,now -fstack-protector-all"

SYMBOLS=-g
OPTIMIZE=-O2
CPP_OPTIONS="$SECURITY_CPP -std=c++17"
COMPILE_OPTIONS="$SYMBOLS $OPTIMIZE $MLP $SECURITY_COMPILE"
OPTIONS="$COMPILE_OPTIONS $SECURITY_CPP"
LINK_OPTIONS="$SECURITY_LINK"

