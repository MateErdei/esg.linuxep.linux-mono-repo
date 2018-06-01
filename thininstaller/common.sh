##
## ALWAYS EDIT //version/product/savlinux/3rdparty/support/common.sh
##
function failure()
{
    echo "FAILURE - $*" | tee -a $LOG | tee -a /tmp/build.log

    # chmod at the end so scaffold can delete its files
    if [[ "$unamestr" == "HP-UX" ]]; then
        chown -R bldsav "$BASE"
    fi
    exit 1
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

# pipefail is only supported on bash 3 and later; some build
# machines have 2.
if ((BASH_VERSINFO[0] > 2)); then
    set -o pipefail
fi

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

MLP=
EXTRA_LIBS=

if [[ "$unamestr" == "HP-UX" ]]; then
    MLP=-mlp64
    EXTRA_LIBS=-L/usr/lib/hpux64
fi

AIX=
if [[ "$unamestr" == "AIX" ]]; then
    # Ensure open64() etc appear in headers without
    # crazy ifdef's redefining the word 'open'
    LARGEFILES=-D_LARGE_FILE_API
    # Work around http://wiki.buici.com/xwiki/bin/view/Programing+C+and+C%2B%2B/Autoconf+and+RPL_MALLOC
    export ac_cv_func_malloc_0_nonnull=yes
fi

SECURITY_CPP=
SECURITY_COMPILE=
SECURITY_LINK=
PENTIUM3_COMPILE=

if [[ "$PLATFORM" == "linux" ]]
then
    SECURITY_CPP="-D_FORTIFY_SOURCE=2"
    SECURITY_COMPILE="-fstack-protector-all"
    PENTIUM3_COMPILE="-march=i686 -mtune=pentium3"
    SECURITY_LINK="-Wl,-z,relro,-z,now -fstack-protector-all"
fi

SYMBOLS=-g
OPTIMIZE=-O2
CPP_OPTIONS="$SECURITY_CPP $LARGEFILES"
COMPILE_OPTIONS="$SYMBOLS $OPTIMIZE $MLP $SECURITY_COMPILE"
OPTIONS="$COMPILE_OPTIONS $SECURITY_CPP $LARGEFILES"
LINK_OPTIONS="$MLP $EXTRA_LIBS $SECURITY_LINK"

# Without this, gunzip isn't found for some reason.
if [[ $BUILDARCH == "HP-UX-ia64" ]]; then
    export PATH=/usr/contrib/bin:$PATH
fi
