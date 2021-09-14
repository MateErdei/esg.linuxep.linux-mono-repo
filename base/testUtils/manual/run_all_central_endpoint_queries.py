#!/usr/bin/env python3

import argparse
import os
import time
import sys
import json


## Before importing cloudClient!
os.environ.setdefault("TMPROOT", "/tmp")


try:
    # running from IDE / for IDE to be able to find libs
    from testUtils.SupportFiles.CloudAutomation import cloudClient
except:
    try:
        # running this script in place from within testUtils
        sys.path.append('../SupportFiles/CloudAutomation')
        import cloudClient
    except:
        # try:
            # running this script inplace but from another dir
        this_file = os.path.realpath(__file__)
        script_dir = os.path.dirname(this_file)
        cloud_automation_dir = os.path.realpath(os.path.join(script_dir, "../SupportFiles/CloudAutomation"))
        if os.path.isdir(cloud_automation_dir):
            sys.path.append(cloud_automation_dir)
        import cloudClient

class Options(object):
    def copy(self):
        o = Options()
        o.__dict__ = self.__dict__.copy()
        return o


def get_api_options(client_id, client_secret, region):
    options = get_options(None, None, region)
    options.client_id = client_id
    options.client_secret = client_secret
    return options


def get_api_client(client_id, client_secret, region):
    options = get_api_options(client_id, client_secret, region)
    return cloudClient.CloudClient(options)


def get_options(email, password, region):
    options = Options()
    options.wait_for_host = False
    options.ignore_errors = False
    options.hostname = None
    options.wait = 30
    options.client_id = None
    options.client_secret = None
    options.email = email
    options.password = password
    options.region = region
    options.proxy = None
    options.proxy_username = None
    options.proxy_password = None
    options.cloud_host = None
    options.cloud_ip = None
    return options


def get_client(email, password, region):
    options = get_options(email, password, region)
    return cloudClient.CloudClient(options)


def add_options():
    parser = argparse.ArgumentParser(description='Runs Live Queries via Central')
    parser.add_argument('-i', '--client-id', required=False, action='store', help="Central account API client ID")
    parser.add_argument('-s', '--secret', required=True, action='store', help="Central account API client secret to use to run live queries")
    parser.add_argument('-r', '--region', required=True, action='store', help="Central region (q, d, p)")
    parser.add_argument('-m', '--machine', required=True, action='store', help="Hostname of the target machine")
    return parser

def main():
    parser = add_options()
    args = parser.parse_args()
    client = get_api_client(args.client_id, args.secret, args.region)
    print("Initial query check showing the hostname of the machine to be queried:")
    response = client.run_live_query_and_wait_for_response("test", "SELECT system_info.hostname, system_info.local_hostname FROM system_info;", args.machine)
    print(response)

    response_str = client.get_all_saved_queries()
    response = json.loads(response_str)

    print("----------------------------------------------")
    print("----------------------------------------------")
    print("----------------------------------------------")
    print("Everything should be working ok if we got here.")
    time.sleep(5)
    print("\nHere is a summary of all the canned queries:")
    for item in response["items"]:
        if "linuxServer" in item["supportedOSes"]:
            if len(item["variables"]) > 0:
                print(f"WARNING will skip: {item['name']}, it needs variables to be set manually.")
            else:
                print(item['name'])

    print("\nWill run all queries in 20 seconds, ctrl-c to quit this if you want to stop and read the above.")
    for i in range(-20, 0):
        time.sleep(1)
        print(abs(i))

    if "items" not in response:
        print(response)
        print("Bad response - it does not contain 'items'")
        return 1

    results_dir = "canned_query_results"
    os.makedirs(results_dir, exist_ok=True)
    for item in response["items"]:
        if "linuxServer" in item["supportedOSes"]:
            print(item["name"])
            if len(item["variables"]) > 0:
                print("Skipping because query needs variables set")
                file_name = os.path.join(results_dir, f"SKIPPED-{item['code']}")
                with open(file_name, "w") as result_file:
                    result_file.write(json.dumps({"name": item["name"], "result": f"Skipped, requires these variables to be set: {item['variables']}"}, indent=4))
            else:
                time.sleep(2)
                canned_query_response = client.run_canned_query_and_wait_for_response(item["id"], hostname=args.machine)
                print(canned_query_response)
                file_name = os.path.join(results_dir, str(item["code"]))
                with open(file_name, "w") as result_file:
                    result_file.write(json.dumps({"name": item["name"], "result": canned_query_response}, indent=4))

    print("Done")
    print(f"Please look in: {results_dir} for all results")

    return 0


if __name__ == "__main__":
    sys.exit(main())
