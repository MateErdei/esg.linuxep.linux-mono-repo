#!/bin/sh
mkdir -p /tmp_test/clean_file_writer
while true
do
    echo "clean" > /tmp_test/clean_file_writer/clean.txt
    sleep  0.1
done