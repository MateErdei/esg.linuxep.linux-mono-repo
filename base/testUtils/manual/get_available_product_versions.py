#!/usr/bin/env python3

import argparse
from datetime import datetime
from datetime import timedelta
import os
import sys


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


def get_api_options(client_id, client_secret, region, hostname):
    options = get_options(None, None, region)
    options.client_id = client_id
    options.client_secret = client_secret
    options.hostname = hostname
    return options


def get_api_client(client_id, client_secret, region, hostname):
    options = get_api_options(client_id, client_secret, region, hostname)
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
    client = get_api_client(args.client_id, args.secret, args.region, args.machine)
    expiresFrom = datetime.now() + timedelta(days=1)
    releases = client.getReleases(expiresFrom.strftime("%Y-%m-%d"))
    print(f"Available product releases: ")
    for release in releases:
        print(f"   Name: {release['name']}, ID: {release['id']}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
