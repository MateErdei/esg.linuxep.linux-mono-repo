#!/bin/bash

JOB_ID=$1
shift
function failure()
{
    echo "$@"
    exit 1
}

function generate_uuid()
{
    local N B T
    for (( N=0; N < 16; ++N ))
    do
        B=$(( $RANDOM%255 ))
        if (( N == 6 ))
        then
            printf '4%x' $(( B%15 ))
        elif (( N == 8 ))
        then
            local C='89ab'
            printf '%c%x' ${C:$(( $RANDOM%${#C} )):1} $(( B%15 ))
        else
            printf '%02x' $B
        fi
        for T in 3 5 7 9
        do
            if (( T == N ))
            then
                printf '-'
                break
            fi
        done
    done
    echo
}

[[ -n "$TEST_PASS_UUID" ]] || TEST_PASS_UUID=$(generate_uuid)
export TEST_PASS_UUID

SCRIPT_DIR="${0%/*}"
[[ $SCRIPT_DIR == $0 ]] && SCRIPT_DIR=.
cd $SCRIPT_DIR


export TEST_TAR=./sspl-test-${JOB_ID}.tgz
TAR_BASENAME=$(basename ${TEST_TAR})
## Gather files
if [[ -z "$SKIP_GATHER" ]]
then
    bash ./gather.sh || failure "Failed to gather test files: $?"
fi
[[ -f "$TEST_TAR" ]] || failure "Failed to gather test files: $TEST_TAR doesn't exist"

## Create template

function compress()
{
    python -c "import json,sys;i=open(sys.argv[1]);o=open(sys.argv[2],'w');json.dump(json.load(i),o,separators=(',',':'))" \
        "$1" \
        "$2" || failure "Unable to compress template $1: $?"
}

TAR_DESTINATION_FOLDER="s3://sspl-testbucket/sspl"
## Upload test tarfile to s3
if [[ -z "$SKIP_TAR_COPY" ]]
then
    aws s3 cp "$TEST_TAR" ${TAR_DESTINATION_FOLDER}/${TAR_BASENAME} || failure "Unable to copy test tarfile to s3"
fi
rm $TEST_TAR