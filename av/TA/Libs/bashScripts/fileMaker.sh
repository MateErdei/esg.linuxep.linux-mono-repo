#!/bin/sh
mkdir -p /tmp_test/file_maker
for x in $(seq 1 1 $1)
do
        echo "I am not an empty file so please spend some time scanning me" > /tmp_test/file_maker/file$x
done