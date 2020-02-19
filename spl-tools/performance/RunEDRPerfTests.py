# depending on arguments run: gcc build, local live queries, central live queries via API

# For each of the below tests / tasks one will be run at a time then exit, ensure we time them and then post the results.
    #Posting from python:
    # import requests
    # url = 'https://www.w3schools.com/python/demopage.php'
    # myobj = {'somekey': 'somevalue'}
    # x = requests.post(url, data = myobj)
    # print(x.text)

#Where to post them:
    # END=$(date +%s)
    # DURATION=$((END-START))
    #
    # # Product details
    # BUILD_DATE=$(grep BUILD_DATE /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
    # PRODUCT_VERSION=$(grep PRODUCT_VERSION /opt/sophos-spl/base/VERSION.ini | awk -F"=" '{print $2}'  | sed -e 's/^[[:space:]]*//')
    #
    # DATETIME=$(env TZ=UTC date "+%Y/%m/%d %H:%M:%S")
    # echo '{"datetime":"'$DATETIME'", "hostname": "'$HOSTNAME'", "build_date":"'$BUILD_DATE'", "product_version":"'$PRODUCT_VERSION'", "eventname":"GCC Build", "start":'$START', "finish":'$END', "duration":'$DURATION'}' > gcc.json
    #
    # curl -X POST "sspl-perf-mon:9200/perf-custom/_doc?pretty" -H 'Content-Type: application/json' -d @gcc.json


#GCC:
# run this script: run-test-gcc.sh which will be: /root/run-test-gcc.sh

#local live query:
#python3 RunLiveQuery.py (run this and it will print usage text)

# Run API central live queries:
#....




