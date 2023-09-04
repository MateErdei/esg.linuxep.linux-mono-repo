#!/usr/bin/env python3

import argparse
import logging
import os
import sys

from PerformanceResources import Jenkins_Job_Return_Code

# Before importing cloudClient!
os.environ.setdefault("TMPROOT", "/tmp")

import cloudClient


class Options(object):
    def copy(self):
        o = Options()
        o.__dict__ = self.__dict__.copy()
        return o


def get_api_client(client_id, client_secret, region):
    options = get_options(None, None, region)
    options.client_id = client_id
    options.client_secret = client_secret

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
    parser.add_argument('-p', '--password', required=True, action='store',
                        help="Central account API client secret or password to use to change on access settings")
    parser.add_argument('-r', '--region', required=True, action='store', help="Central region (q, d, p)")

    parser.add_argument('--on-access-read', action='store_true', help="Toggle On-Access On-Read on")
    parser.add_argument('--no-on-access-read', dest='on-access-read', action='store_false',
                        help="Toggle On-Access On-Read off")
    parser.add_argument('--on-access-write', action='store_true', help="Toggle On-Access On-Write on")
    parser.add_argument('--no-on-access-write', dest='on-access-write', action='store_false',
                        help="Toggle On-Access On-Write off")

    return parser


def main():
    parser = add_options()
    args = parser.parse_args()
    on_read_toggle, on_write_toggle = args.on_access_read, args.on_access_write
    client = get_api_client(args.client_id, args.password, args.region)

    # API information came from the following two links:
    # https://developer.sophos.com/endpoint-policy-settings-all#server-threat-protection
    # https://developer.sophos.com/docs/endpoint-v1/1/overview

    policy_type = "server-threat-protection"
    on_read_setting_name = "endpoint.threat-protection.malware-protection.on-access.scan-on-read.enabled"
    on_write_setting_name = "endpoint.threat-protection.malware-protection.on-access.scan-on-write.enabled"
    body = {on_read_setting_name: {"value": on_read_toggle}, on_write_setting_name: {"value": on_write_toggle}}

    logging.basicConfig()
    logging.getLogger().setLevel(logging.DEBUG)
    try:
        # patch_response will be a dictionary of the updated policy, can be used for a sanity check

        # Can change policy by using policy id or policy type
        # policy_id = client.get_policy_id_by_type(policy_type)
        # logging.info(f"Policy id: {policy_id}")
        # patch_response = client.patch_policy_by_id(policy_id, body)

        patch_response = client.patch_policy_by_type(policy_type, body)

        if patch_response[on_write_setting_name]["value"] != on_write_toggle or patch_response[on_read_setting_name]["value"] != on_read_toggle:
            # In a weird state here as requests library did not throw an exception (such as unauthorized, timeout, etc.),
            # but the updated policy we got back did not contain the values we expect. Not obvious what went wrong or
            # if anything went wrong at all
            logging.error("Something unexpected may have gone wrong when patching policy")
            logging.error(f"Policy settings we got back - \n"
                          f"{on_write_setting_name}: {patch_response[on_write_setting_name]}\n"
                          f"{on_read_setting_name}: {patch_response[on_read_setting_name]}")
            return Jenkins_Job_Return_Code.UNSTABLE
    except Exception as e:
        logging.error(f"Patching policy failed, patch details: {body}, error: {e}")
        return Jenkins_Job_Return_Code.FAILURE

    return Jenkins_Job_Return_Code.SUCCESS


if __name__ == "__main__":
    sys.exit(main())
