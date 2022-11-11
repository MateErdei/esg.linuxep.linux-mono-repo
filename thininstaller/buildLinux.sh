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

INPUT=/build/input

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

REDIST=/build/redist
mkdir -p $REDIST


## These can't be exitFailure since it doesn't exist till the sourcing is done
[ -f "$BASE"/build-scripts/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
source "$BASE"/build-scripts/pathmgr.sh
[ -f "$BASE"/build-scripts/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
source "$BASE"/build-scripts/common.sh


function untar_input()
{
    local input=$1
    local tar=${INPUT}/${input}.tar

    if [[ -f $tar ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    else
      local targz=${INPUT}/${input}.tar.gz
      if [[ -f $targz ]]
      then
        echo "Untaring $targz"
        tar xvf "$targz" -C "$REDIST"
      else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
      fi
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
        if [[ -f "$INPUT/cmake/bin/cmake" ]]
        then
            cp -rf $INPUT/cmake $REDIST && \
            addpath "$REDIST/cmake/bin"
            chmod 700 $REDIST/cmake/bin/cmake || exitFailure "Unable to chmod cmake"
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
        untar_input cmcsrouterapi
        untar_input pluginapi

        if [[ -f ${INPUT}/update_certs/ps_rootca.crt ]]
        then
          # copy prod certs into expected location.
          cp ${INPUT}/update_certs/ps_rootca.crt ./ps_rootca.crt
          cp ${INPUT}/update_certs/ps_rootca.crt ./rootca.crt
        else
          exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Missing cert inputs"
        fi
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


    rm -rf $BASE/output
    rm -rf $BASE/installer
    mkdir -p ${BASE}/build
#   Required for build scripts to run on dev machines
    export LIBRARY_PATH=/build/input/gcc/lib64/:${LIBRARY_PATH}:/usr/lib/x86_64-linux-gnu
    export CPLUS_INCLUDE_PATH=/build/input/gcc/include/:/usr/include/x86_64-linux-gnu/:${CPLUS_INCLUDE_PATH}
    export CPATH=/build/input/gcc/include/:${CPATH}
    installer_dir=$BASE/installer
    output=$BASE/output
    pushd ${BASE}/build
    chmod +x "$INPUT/cmake/bin/cmake"
    "$INPUT/cmake/bin/cmake" -DREDIST="${REDIST}" -DINSTALLERDIR="${installer_dir}" -DOUTPUT="${output}" ..
    make || exitFailure 15 "Failed to build thininstaller"
    make copyInstaller || exitFailure $FAILED_TO_COPY_INSTALLED "Failed to copy installer"
    popd

    echo "Bundled libs:"
    ls -l installer/lib64

    mkdir -p output
    tar cf partial_installer.tar installer/*
    rm -f partial_installer.tar.gz
    gzip partial_installer.tar

    output_install_script="SophosSetup.sh"
    cat $INSTALLER_HEADER partial_installer.tar.gz > output/${output_install_script}
    rm -f partial_installer.tar.gz

    pushd output
    chmod u+x ${output_install_script}
    LD_LIBRARY_PATH= \
        sb_manifest_sign --folder . --output manifest.dat --legacy
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
exit $EXIT
