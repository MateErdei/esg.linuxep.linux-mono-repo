import logging
import requests
import sys
import time
import socket

import argparse
import subprocess

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
logger = logging.getLogger("EDRPerformance")
def add_options():
    parser = argparse.ArgumentParser(description='Performance test runner for EDR')
    parser.add_argument('-s', '--suite', default='gcc', action='store',
                        choices=['gcc', 'local-livequery', 'central-livequery'],
                        help="Select which performance test suite to run")
    parser.add_argument('--test-script-path', default='/root/run-test-gcc.sh', action='store',
                        help='path to the test script')
    parser.add_argument('--hostname', default='None', action='store')
    return parser




def get_part_after_equals(key_value_pair):
    parts = key_value_pair.strip().split(" = ")
    if len(parts) > 1:
        return parts[1]
    return ""


def run_test(args_list):
    start_time = int(time.time())
    print("runnig process")
    subprocess.run(args_list, timeout=120)
    end_time = int(time.time())

    duration = end_time - start_time
    date_time = time.strftime("%Y/%m/%d %H:%M:%S")
    hostname = socket.gethostname()

    build_date = "unknown"
    product_version = "unknown"

    with open("/opt/sophos-spl/base/VERSION.ini", 'r') as version_file:
        for line in version_file:
            if "BUILD_DATE" in line:
                build_date = get_part_after_equals(line)
            if "PRODUCT_VERSION" in line:
                product_version = get_part_after_equals(line)

    json_data = '{"datetime":"'+date_time+'", "hostname": "'+hostname+'", "build_date":"'+build_date \
                +'", "product_version":"'+product_version+'", "eventname":"GCC Build", "start":'+ str(start_time) \
                + ', "finish":'+ str(end_time) +', "duration":'+str(duration) +'}'
    print(json_data)
    #r = requests.post('sspl-perf-mon:9200/perf-custom/_doc?pretty', json={"key": "value"})

def main(argv):
    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    parser = add_options()
    args = parser.parse_args()

    if args.suite == 'gcc':
        run_test(['bash', "build_gcc_only.sh"])
    if args.suite == 'local-livequery':
        run_test(['python3', 'RunLiveQuery.py', 'basic_query', '"SELECT * FROM processes;"'])
    if args.suite == 'central-livequery':
        run_test(['python3', 'cloudClient.py', '-region', 'q', '--email', 'darwin@sophos', '--password',
                        'DarwinCh1pm0nk', 'run_live_query_and_wait_for_response', 'basic_query2', '"SELECT * FROM processes;"', args.hostname])



if __name__ == '__main__':
    sys.exit(main(sys.argv))