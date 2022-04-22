
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

set -o pipefail

mkdir -p log
LOG=$BASE/log/build.log
date | tee -a $LOG | tee /tmp/build.log

unamestr=$(uname)
PLATFORM=`uname -s | LC_ALL=C tr '[:upper:]' '[:lower:]' | LC_ALL=C tr -d '-'`
cpustr=$(uname -p)
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
