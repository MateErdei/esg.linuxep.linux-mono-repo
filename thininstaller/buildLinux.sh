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
INPUT=$BASE/input
ALLEGRO_REDIST=/redist/binaries/linux11/input
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
    sed s/VERSION_REPLACEMENT_STRING/$VERSION/g ./installer_header.sh > installer_header_versioned.sh
    INSTALLER_HEADER=installer_header_versioned.sh
}

function untar_or_link_to_redist()
{
    local input=$1
    local tar=${INPUT}/${input}.tar

    if [[ -f $tar ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    elif [[ -d ${ALLEGRO_REDIST}/$input ]]
    then
        echo "Linking ${REDIST}/$input to ${ALLEGRO_REDIST}/$input"
        ln -snf ${ALLEGRO_REDIST}/$input ${REDIST}/$input
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
        elif [[ -d $ALLEGRO_REDIST ]]
        then
            ln -snf $ALLEGRO_REDIST/openssl $REDIST/openssl
            echo "Failed to find input openssl, using allegro redist openssl"
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
        elif [[ -d $ALLEGRO_REDIST/cmake ]]
        then
            ln -snf $ALLEGRO_REDIST/cmake $REDIST/cmake
            addpath "$REDIST/cmake/bin"
        else
            echo "WARNING: using system cmake"
        fi
        echo "Using cmake: $(which cmake)"

        untar_or_link_to_redist versig
        untar_or_link_to_redist curl
        untar_or_link_to_redist SUL
        untar_or_link_to_redist boost
        untar_or_link_to_redist expat
        untar_or_link_to_redist zlib
    elif [[ -d $ALLEGRO_REDIST ]]
    then
        echo "WARNING: No input available; using system or /redist files"
        REDIST=$ALLEGRO_REDIST
        addpath "$REDIST/cmake/bin"
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

    prepare_dependencies
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
