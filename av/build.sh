#!/usr/bin/env bash
PRODUCT=ExamplePlugin

FAILURE_DIST_FAILED=18
FAILURE_COPY_SDDS_FAILED=60
FAILURE_INPUT_NOT_AVAILABLE=50

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
[[ -z "${CLEAN:-}" ]] && CLEAN=1

INPUT=$BASE/input
ALLEGRO_REDIST=/redist/binaries/linux11/input

function untar_or_link_to_redist()
{
    local input=$1
    local tarbase=$2
    local tar
    if [[ -n $tarbase ]]
    then
        tar=${INPUT}/${tarbase}.tar
    else
        tar=${INPUT}/${input}.tar
    fi

    if [[ -f "$tar" ]]
    then
        echo "Untaring $tar"
        tar xf "$tar" -C "$REDIST"
    elif [[ -f "${tar}.gz" ]]
    then
        echo "untaring ${tar}.gz"
        tar xzf "${tar}.gz" -C "$REDIST"
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
    [ -f "$BASE"/build-files/pathmgr.sh ] || { echo "Can't find pathmgr.sh" ; exit 10 ; }
    source "$BASE"/build-files/pathmgr.sh
    [ -f "$BASE"/build-files/common.sh ] || { echo "Can't find common.sh" ; exit 11 ; }
    source "$BASE"/build-files/common.sh

    echo "STARTINGDIR=$STARTINGDIR"
    echo "BASE=$BASE"
    echo "PATH=$PATH"
    echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH:-unset}"

    cd $BASE

    unpack_scaffold_gcc_make "$INPUT"

    if [[ -d $INPUT ]]
    then
        REDIST=$BASE/redist
        mkdir -p $REDIST

        untar_or_link_to_redist pluginapi
        untar_or_link_to_redist cmake cmake-3.11.2-linux
        untar_or_link_to_redist log4cplus

    elif [[ -d "$ALLEGRO_REDIST" ]]
    then
        echo "WARNING: No input available; using system or $ALLEGRO_REDIST files"
        REDIST=$ALLEGRO_REDIST
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No redist or input available"
    fi
    addpath "$REDIST/cmake/bin"

    [[ -n $CC ]] || CC=$(which gcc)
    [[ -n $CXX ]] || CXX=$(which g++)
    export CC
    export CXX

    [[ $CLEAN == 1 ]] && rm -rf build${BITS}
    mkdir -p build${BITS}
    cd build${BITS}
    [[ -n ${NPROC:-} ]] || NPROC=2
    cmake -v -DREDIST="${REDIST}" -DINPUT="${REDIST}" \
        -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC \
        .. || exitFailure 14 "Failed to configure $PRODUCT"
    make -j${NPROC} CXX=$CXX CC=$CC || exitFailure 15 "Failed to build $PRODUCT"

    make install CXX=$CXX CC=$CC || exitFailure 17 "Failed to install $PRODUCT"
    make dist CXX=$CXX CC=$CC ||  exitFailure $FAILURE_DIST_FAILED "Failed to create dist $PRODUCT"
    cd ..

    rm -rf output
    mkdir -p output
    echo "STARTINGDIR=$STARTINGDIR" >output/STARTINGDIR
    echo "BASE=$BASE" >output/BASE
    echo "PATH=$PATH" >output/PATH
    echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >output/LD_LIBRARY_PATH

    cp -a build64/sdds output/SDDS-COMPONENT || exitFailure $FAILURE_COPY_SDDS_FAILED "Failed to copy SDDS component to output"

    echo "Build Successful"
    return 0
}

build 64 2>&1 | tee -a $LOG
EXIT=$?
cp $LOG $OUTPUT/ || true
exit $EXIT
