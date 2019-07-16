#!/usr/bin/env bash

# Run and time GCC build, send data back to elastic search.
function do_gcc_test()
{
    START=$(date +%s)

    pushd /home/pair/gcc-build-test/gcc-gcc-6_4_0-release || exit 1

    rm -rf ./build
    mkdir build
    cd build
    ../configure --enable-languages=c,c++ --disable-multilib

    #TODO RE-ENABLE
    #make -j4 || exit 1


    END=$(date +%s)
    DURATION=$((END-START))

    # Product details
    local dir="/opt/sophos-spl"
    if [[ -d "$dir" ]]
    then
        BUILD_DATE=$(grep BUILD_DATE /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
        PRODUCT_VERSION=$(grep PRODUCT_VERSION /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
    else
        BUILD_DATE="none"
        PRODUCT_VERSION="none"
    fi


    echo "Product details:"
    echo "BUILD_DATE: $BUILD_DATE"
    echo "PRODUCT_VERSION: $PRODUCT_VERSION"


    DATETIME=$(env TZ=UTC date "+%Y/%m/%d %H:%M:%S")
    echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"event1", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > gcc.json

    echo "---"
    cat gcc.json
    echo "---"

    curl -X POST "sspl-perf-mon:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @gcc.json

    popd

}


# Run and time copying files from and to test vm server, send results back to elastic.
function do_copy_test()
{

    START=$(date +%s)

    pushd /home/pair/copy_test_files || exit 1
    scp -r pair@lind-server1:/home/pair/CopyDown . || exit 1
    scp -r ./* pair@lind-server1:/home/pair/CopyUp || exit 1
    # clean up file copying for next run.
    ssh pair@lind-server1 "rm -rf /home/pair/CopyUp/*" || exit 1
    rm -rf *
    popd


    END=$(date +%s)
    DURATION=$((END-START))

    # Product details
    local dir="/opt/sophos-spl"
    if [[ -d "$dir" ]]
    then
        BUILD_DATE=$(grep BUILD_DATE /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
        PRODUCT_VERSION=$(grep PRODUCT_VERSION /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
    else
        BUILD_DATE="none"
        PRODUCT_VERSION="none"
    fi


    DATETIME=$(env TZ=UTC date "+%Y/%m/%d %H:%M:%S")
    echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"event2", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > copy.json

    echo "---"
    cat copy.json
    echo "---"

    curl -X POST "10.55.37.63:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @copy.json


}

RUNS=2

for i in $(seq 1 $RUNS)
    do
    do_gcc_test
done

for i in $(seq 1 $RUNS)
    do
    do_copy_test
done

