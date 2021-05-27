#!/bin/bash

# T44677743
# scan a folder where files are being created and deleted

cont=/tmp/continue
touch $cont
DIR=/tmp/test_dir
mkdir -p $DIR

function create_delete_files()
{
    while [[ -f $cont ]]
    do
        for i in $(seq 1 1000)
        do
            touch $DIR/test-$i
        done
        for i in $(seq 1 1000)
        do
            rm -f $DIR/test-$i
        done
    done
}

create_delete_files &

while [[ -f $cont ]]
do
    avscanner $DIR
    RET=$?
    case $RET in
        0)
            echo "avscanner success"
            ;;
        8)
            echo "avscanner completed with errors"
            ;;
        *)
            rm -rf $cont $DIR
            echo "AVSCANNER FAILED: $RET"
            ;;
    esac
done
