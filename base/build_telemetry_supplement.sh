#!/usr/bin/env bash
source /etc/profile
set -ex
set -o pipefail

FAILURE_INPUT_NOT_AVAILABLE=50
FAILURE_BAD_ARGUMENT=53

STARTINGDIR=$(pwd)

cd ${0%/*}
BASE=$(pwd)
export BASE
OUTPUT=$BASE/output
export OUTPUT

LOG=$BASE/log/build.log
rm -f $LOG
mkdir -p $BASE/log || exit 1
rm -rf $OUTPUT
mkdir -p $OUTPUT || exit 2

INPUT=$BASE/input
ALLEGRO_REDIST=/redist/binaries/linux11/input
#PLUGIN_TAR=

while [[ $# -ge 1 ]]
do
    case $1 in
#        --plugin-api-tar|--pluginapi)
#            shift
#            PLUGIN_TAR=$1
#            ;;
        --generate-sdds-import)
            shift
            GENERATE_SDDS_IMPORT=$1
            ;;
        --generate-manifest-dat)
            shift
            GENERATE_MANIFEST_DAT=$1
            ;;
        *)
            exitFailure ${FAILURE_BAD_ARGUMENT} "unknown argument $1"
            ;;
    esac
    shift
done


function exitFailure()
{
    local CODE=$1
    shift
    echo "FAILURE - $*" | tee -a $LOG | tee -a /tmp/build_telemetry_config.log
    exit $CODE
}

function copy_or_link_to_redist()
{
    local input_component=$1
    local filename=$2

    #local tar
    local file_path

    file_path=${INPUT}/${input_component}/${filename}

    if [[ -f "$file_path" ]]
    then
        echo "Using input file $file_path"
    elif [[ -d "${ALLEGRO_REDIST}/$input" ]]
    then
        echo "Linking ${REDIST}/$input to ${ALLEGRO_REDIST}/$input"
        ln -snf "${ALLEGRO_REDIST}/$input" "${REDIST}/$input"
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
    fi
}

#    if [[ -n $tarbase ]]
#    then
#        tar=${INPUT}/${tarbase}.tar
#    else
#        tar=${INPUT}/${input}.tar
#    fi

#    if [[ -n "$override_tar" ]]
#    then
#        echo "Untaring override $override_tar"
#        tar xzf "${override_tar}" -C "$REDIST"
#    elif [[ -f "$tar" ]]
#    then
#        echo "Untaring $tar"
#        tar xf "$tar" -C "$REDIST"
#    elif [[ -f "${tar}.gz" ]]
#    then
#        echo "untaring ${tar}.gz"
#        tar xzf "${tar}.gz" -C "$REDIST"
#    elif [[ -d "${ALLEGRO_REDIST}/$input" ]]
#    then
#        echo "Linking ${REDIST}/$input to ${ALLEGRO_REDIST}/$input"
#        ln -snf "${ALLEGRO_REDIST}/$input" "${REDIST}/$input"
#    else
#        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "Unable to get input for $input"
#    fi
#}

function build()
{
    if [[ -d $INPUT ]]
    then
        REDIST=$BASE/redist
        mkdir -p $REDIST

        #copy_or_link_to_redist telemetry telemetry-config.json
        if [[ -f ${INPUT}/telemetry/telemetry-config.json ]]
        then
            mkdir -p ${REDIST}/telemetry
            cp "${INPUT}/telemetry/telemetry-config.json" "${REDIST}/telemetry/"
        else
            exitFailure $FAILURE_INPUT_NOT_AVAILABLE "telemetry-config.json not available"
        fi

    elif [[ -d "$ALLEGRO_REDIST" ]]
    then
        echo "WARNING: No input available; using system or $ALLEGRO_REDIST files"
        REDIST=$ALLEGRO_REDIST
    else
        exitFailure $FAILURE_INPUT_NOT_AVAILABLE "No redist or input available"
    fi

    local DISTRIBUTION_TOOL_DIR=${BASE}/products/distribution


    [[ -n "${GENERATE_SDDS_IMPORT}" ]] || GENERATE_SDDS_IMPORT=${DISTRIBUTION_TOOL_DIR}/generateSDDSImport.py
    export GENERATE_SDDS_IMPORT

    [[ -n "${GENERATE_MANIFEST_DAT}" ]] || GENERATE_MANIFEST_DAT=${DISTRIBUTION_TOOL_DIR}/generateManifestDat.py
    export GENERATE_MANIFEST_DAT

    bash -x $BASE/build/create_telemetry_supplement_component.sh $REDIST

    echo "Build Completed"
}

build | tee -a $LOG
cp -f $LOG $OUTPUT/

exit 0
