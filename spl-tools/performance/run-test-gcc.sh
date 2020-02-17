#!/bin/bash

# Script to run and time GCC builds on performance soak test machines.
# The results are saved in elastic search in the index perf-custom.
# This script requires the following dir structure:
# /home/pair/gcc-build-test/gcc-gcc-6_4_0-release (this is the unpacked contents of gcc-gcc-6_4_0-release.tar.gz
# which can be found here: /mnt/filer6/linux/SSPL/tools/gcc-gcc-6_4_0-release.tar.gz)

START=$(date +%s)

pushd /home/pair/gcc-build-test || exit 1
rm -rf ./build
mkdir build
cd build
../gcc-gcc-6_4_0-release/configure --enable-languages=c,c++ --disable-multilib
make -j4 || exit 1
popd

END=$(date +%s)
DURATION=$((END-START))

# Product details
BUILD_DATE=$(grep BUILD_DATE /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
PRODUCT_VERSION=$(grep PRODUCT_VERSION /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')

DATETIME=$(env TZ=UTC date "+%Y/%m/%d %H:%M:%S")
echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"GCC Build", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > gcc.json

curl -X POST "sspl-perf-mon:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @gcc.json
