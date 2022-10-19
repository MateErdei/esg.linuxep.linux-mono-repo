#!/usr/bin/env bash
CC=${CC:-gcc}
LIB=$($CC -print-file-name=$1)

echo $CC $LIB $1 >>/tmp/x

[[ -f $LIB ]] || { echo "Failed to locate $1" ; exit 1 ; }

while true
do
    X=$(readlink "$LIB")
    if [[ $? != 0 ]]
    then
        echo $LIB
        exit 0
    fi
    case $X in
        /*)
            LIB=$X
            ;;
        *)
            LIB=$(dirname $LIB)/$X
            ;;
    esac
done
