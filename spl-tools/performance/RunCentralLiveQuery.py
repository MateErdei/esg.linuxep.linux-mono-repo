#!/usr/bin/env python3

import argparse
import logging
import json
import os
import sys
import time

from PerformanceResources import get_current_unix_epoch_in_seconds, Jenkins_Job_Return_Code

# Before importing cloudClient!
os.environ.setdefault("TMPROOT", "/tmp")

# from . import cloudClient
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
    parser.add_argument('-e', '--email', required=False, action='store', help="Central account email address to use")
    parser.add_argument('-p', '--password', required=True, action='store', help="Central account API client secret or password to use to run live queries")
    parser.add_argument('-r', '--region', required=True, action='store', help="Central region (q, d, p)")
    parser.add_argument('-n', '--name', required=True, action='store', help="Query name")
    parser.add_argument('-q', '--query', required=True, action='store', help="Query string")
    parser.add_argument('-m', '--machine', required=True, action='store', help="Hostname of the target machine")
    return parser


def main():
    parser = add_options()
    args = parser.parse_args()

    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)

    num_retries = 6
    sleep_amt_between_tries = 10
    client = None
    for _ in range(num_retries):
        try:
            if args.client_id is None:
                client = get_client(args.email, args.password, args.region)
            else:
                client = get_api_client(args.client_id, args.password, args.region)
            break
        except Exception as exception:
            logging.warning(f"Failed to initialize client, error: {exception}")
        time.sleep(sleep_amt_between_tries)

    if client is None:
        # Not a product issue that api client could not be initialized so unstable rather than failure
        return Jenkins_Job_Return_Code.UNSTABLE

    relative_timeout = 120
    for _ in range(num_retries):
        try:
            response = client.run_live_query_and_wait_for_response("test", "SELECT system_info.hostname, system_info.local_hostname FROM system_info;", args.machine, relative_timeout)
            if response is not None:
                break
            logging.warning(f"Test live query failed as response was None")
        except Exception as exception:
            logging.warning(f"Test live query failed with exception: {exception}")
        time.sleep(sleep_amt_between_tries)

    if response is None:
        return Jenkins_Job_Return_Code.UNSTABLE

    try:
        response = response[0][0]
    except Exception as ex:
        return Jenkins_Job_Return_Code.FAILURE

    if args.client_id is None:
        if args.machine not in response:
            return Jenkins_Job_Return_Code.FAILURE
    else:
        if args.machine not in response.values():
            return Jenkins_Job_Return_Code.FAILURE

    start_time = get_current_unix_epoch_in_seconds()
    response = client.run_live_query_and_wait_for_response(args.name, args.query, args.machine, relative_timeout)
    end_time = get_current_unix_epoch_in_seconds()

    result = {"start_time": start_time,
              "end_time": end_time,
              "duration": end_time - start_time,
              "success": True}

    print(json.dumps(result))
    return Jenkins_Job_Return_Code.SUCCESS


if __name__ == "__main__":
    sys.exit(main())
