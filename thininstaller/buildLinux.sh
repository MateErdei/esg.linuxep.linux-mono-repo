#!/bin/bash
PRODUCT=sspl-thininstaller

FAILED_TO_COPY_INSTALLED=16
FAILURE_INPUT_NOT_AVAILABLE=50

source /etc/profile
set -ex
set -o pipefail

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
OUTPUT=$BASE/output
INSTALLER_HEADER=installer_header.sh

LOG=$BASE/log/build.log
mkdir -p $BASE/log || exit 1
export NO_REMOVE_GCC=1

while [[ $# -ge 1 ]]
do
    case $1 in
        --no-build)
            NO_BUILD=1
            ;;
        --no-unpack)
            NO_UNPACK=1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done

INPUT=$BASE/input

if [[ ! -d "$INPUT" ]]
then
    if [[ -d "$BASE/sspl-thininstaller-build" ]]
    then
        INPUT="$BASE/sspl-thininstaller-build/input"
    else
        MESSAGE_PART1="You need to run the following to setup your input folder: "
        MESSAGE_PART2="python3 -m build_scripts.artisan_fetch release-package.xml"
        exitFailure ${FAILURE_INPUT_NOT_AVAILABLE} "${MESSAGE_PART1}${MESSAGE_PART2}"
    fi
fi

REDIST=$BASE/redist
mkdir -p $REDIST


## These can't be exitFailure since it doesn't exist till the sourcing is done
[ -f "$BASE"/build-scripts/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
source "$BASE"/build-scripts/pathmgr.sh
[ -f "$BASE"/build-scripts/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-scripts/common.sh


function create_versioned_installer_header()
{
    local VERSION=$(grep "package name=\"sspl-thininstaller\"" release-package.xml | sed -e s/.*version=\"// -e s/\"\>//)

    # get product version
    local PRODUCT_VERSION=$(python $BASE/build-scripts/computeAutoVersion.py "${VERSION}")
    [[ -n ${PRODUCT_VERSION} ]] || failure 7 "Failed to create PRODUCT_VERSION"

    sed s/VERSION_REPLACEMENT_STRING/${PRODUCT_VERSION}/g ./installer_header.sh > installer_header_versioned.sh
    INSTALLER_HEADER=installer_header_versioned.sh
}

function untar_input()
{
    local input=$1
    local tar=${INPUT}/${input}.tar

    if [[ -f $tar ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

function prepare_dependencies()
{
    echo "Preparing dependencies"

    if [[ -d $INPUT ]]
    then
        unpack_scaffold_gcc_make "$INPUT"

        # openssl
        OPENSSL_TAR=${INPUT}/openssl.tar
        if [[ -f $OPENSSL_TAR ]]
        then
            rm -rf $REDIST/openssl
            tar xf "$OPENSSL_TAR" -C "$REDIST"
            ln -snf libssl.so.1 ${REDIST}/openssl/lib64/libssl.so.11
            ln -snf libcrypto.so.1 ${REDIST}/openssl/lib64/libcrypto.so.11
        else
            exitFailure 12 "Failed to find openssl"
        fi
        export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${REDIST}/openssl/lib64

        # cmake
        local CMAKE_TAR=$(ls $INPUT/cmake-*.tar.gz)
        if [[ -f "$CMAKE_TAR" ]]
        then
            tar xzf "$CMAKE_TAR" -C "$REDIST"
            addpath "$REDIST/cmake/bin"
        else
            echo "WARNING: using system cmake"
        fi
        echo "Using cmake: $(which cmake)"

        untar_input versig
        untar_input curl
        untar_input SUL
        untar_input boost
        untar_input expat
        untar_input zlib
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get dependencies"
    fi
}

function build()
{
    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    cd $BASE

    if [[ -z "$NO_UNPACK" ]]
    then
        prepare_dependencies
    fi

    if [[ ${NO_BUILD} == 1 ]]
    then
        exit 0
    fi

    ls -l ./redist

    rm -rf $BASE/output
    rm -rf $BASE/installer
    mkdir -p ${BASE}/build

    installer_dir=$BASE/installer
    output=$BASE/output
    pushd ${BASE}/build
    cmake -v -DREDIST="${REDIST}" -DINSTALLERDIR="${installer_dir}" -DOUTPUT="${output}" ..
    make || exitFailure 15 "Failed to build thininstaller"
    make copyInstaller || exitFailure $FAILED_TO_COPY_INSTALLED "Failed to copy installer"
    popd

    mkdir -p output
    tar cf partial_installer.tar installer/*
    rm -f partial_installer.tar.gz
    gzip partial_installer.tar

    output_install_script="SophosSetup.sh"
    create_versioned_installer_header
    cat $INSTALLER_HEADER partial_installer.tar.gz > output/${output_install_script}
    rm -f partial_installer.tar.gz

    pushd output
    chmod u+x ${output_install_script}
    LD_LIBRARY_PATH= \
        python ../generateManifestDat.py .
    tar cf installer.tar *
    gzip installer.tar
    sha256=$(python ../sha256OfFile.py installer.tar.gz)
    mv installer.tar.gz "${sha256}.tar.gz"
    echo "{\"name\": \"${sha256}\"}" > latest.json
    rm manifest.dat
    rm ${output_install_script}
    popd

    echo "Build completed"
}

build | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
