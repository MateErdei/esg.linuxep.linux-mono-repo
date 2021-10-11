#!/bin/bash

INCLUDE_DIR=$1
EXCLUDE_DIR=$2
OUTPUT_DIR=$3
TESTS_DIR=$4

for binary in $(find $TESTS_DIR -type f -executable \( -iname "*" ! -iname "*.so" \))
do
    echo $binary
    kcov --include-path ${INCLUDE_DIR} --exclude-path ${EXCLUDE_DIR} ${OUTPUT_DIR} $binary
done

echo "Your output HTML files are here: ${OUTPUT_DIR}"

