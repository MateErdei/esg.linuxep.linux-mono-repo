
#!/usr/bin/env bash

function install_product()
{

  if [[ -f /opt/sophos-spl/bin/uninstall.sh ]]
  then
    uninstall_product
  fi

  if [[ -d /opt/sophos-spl ]]
  then
    rm -rf /opt/sophos-spl
  fi

  scp pair@lind-server1:/home/pair/perform_prereq/SophosSetup-dogfood.sh /tmp/SophosSetup-dogfood.sh

  /tmp/SophosSetup-dogfood.sh

  rm -rf /tmp/SophosSetup-dogfood.sh
}

function uninstall_product()
{
  /opt/sophos-spl/bin/uninstall.sh --force
}

# Run and time GCC build, send data back to elastic search.
function do_gcc_test()
{
    START=$(date +%s)

    pushd /home/pair/gcc-build-test || exit 1
    rm -rf ./build
    mkdir build
    cd build

    ../gcc-gcc-9-branch/configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=/usr/local/gcc-9.1 --enable-checking=release --enable-languages=c,c++,fortran --disable-multilib --program-suffix=-9.1

    make -j4 || exit 1

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


    echo "Product details:"
    echo "BUILD_DATE: $BUILD_DATE"
    echo "PRODUCT_VERSION: $PRODUCT_VERSION"


    DATETIME=$(env TZ=UTC date "+%Y/%m/%d %H:%M:%S")
    echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"build_gcc", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > gcc.json

    echo "---"
    cat gcc.json
    echo "---"

    curl -X POST "sspl-perf-mon:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @gcc.json

    popd

}


# Run and time copying files from and to test vm server, send results back to elastic.
function do_copy_test()
{
    local runs=$1
    START=$(date +%s)

    for i in $(seq 1 ${runs})
    do
      pushd /home/pair/copy_test_files || exit 1

      scp -r pair@lind-server1:/home/pair/CopyDown . || exit 1
      scp -r ./* pair@lind-server1:/home/pair/CopyUp || exit 1
      #clean up file copying for next run.
      ssh pair@lind-server1 "rm -rf /home/pair/CopyUp/*" || exit 1
      rm -rf *
      popd

    done

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
    echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"copy_files", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > copy.json

    echo "---"
    cat copy.json
    echo "---"

    curl -X POST "10.55.37.63:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @copy.json


}

test_runs=$1
test_setup_wait=$2
copy_test_repeats=$3
wait_period=$4


echo "Installing product..."
install_product

# wait for machine to connect to dbos website, currently we have no way to checking when this will happen

echo "Sleeping for ${test_setup_wait} minutes to ensure everything is connected for test"
sleep ${test_setup_wait}m

echo "Running tests with product installed"

for i in $(seq 1 ${test_runs})
do
	do_gcc_test
	do_copy_test ${copy_test_repeats}

	echo "Sleeping for ${wait_period} minutes"
	sleep ${wait_period}m
done

echo "Test runs completed with product installed"

echo "Uninstalling product"

uninstall_product

echo "Running tests with no product installed"

for i in $(seq 1 ${test_runs})
do
        do_gcc_test
        do_copy_test ${copy_test_repeats}

        echo "Sleeping for ${wait_period} minutes"
        sleep ${wait_period}m
done

echo "Test runs completed without product installed"


