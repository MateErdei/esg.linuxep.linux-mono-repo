#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2018-2023 Sophos Limited. All rights reserved.

import http.client
import ssl
import sys


def _create_https_connection():
    # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
    return http.client.HTTPSConnection("localhost", 4443, context=ssl._create_unverified_context())


# filename - XML file to set as policy in fake cloud
# policyType - The type of policy, specidifed by ID e.g. "SAV", "MCS", "ALC"
def setPolicyInFakeCloud(filename, policyType):
    with open(filename, "rb") as f:
        data = f.read()

    headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

    conn = _create_https_connection()
    conn.request("PUT", "/controller/" + policyType + "/policy", data, headers)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    return 0

def getPolicyInFakeCloud(filename, policyType):
    headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

    conn = _create_https_connection()
    conn.request("GET", "/controller/" + policyType + "/policy", "", headers)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()

    output(remote_file, filename)

    return 0

def output(data, filename=None):
    if filename is None:
        print(data)
    else:
        with open(filename,"wb") as f:
            f.write(data)

def setMcsPolicyInFakeCloud(filename):
    return setPolicyInFakeCloud(filename, "mcs")

def getMcsPolicyInFakeCloud(filename):
    return getPolicyInFakeCloud(filename, "mcs")

def setAlcPolicyInFakeCloud(filename):
    return setPolicyInFakeCloud(filename, "alc")

def setSavPolicyInFakeCloud(filename):
    return setPolicyInFakeCloud(filename, "sav")

def sendCmdToFakeCloud(cmd, filename=None):
    conn = _create_https_connection()
    conn.request("GET", "/"+cmd)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    output(remote_file, filename)
    return 0

def sendLiveQueryToFakeCloud(query, command_id):
    headers = {"Content-type": "application/octet-stream",
               "Accept": "text/plain",
               "Command-ID": command_id}

    conn = _create_https_connection()
    conn.request("PUT", "/controller/livequery/command", query, headers)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    print("Set livequery response: {}".format(remote_file))
    return 0


def sendResponseActionToFakeCloud(query, command_id, return400=False):
    headers = {"Content-type": "application/octet-stream",
               "Accept": "text/plain",
               "Command-ID": command_id}
    if return400:
        headers["X-Response-Code"] = "400"

    conn = _create_https_connection()
    conn.request("PUT", "/controller/core/command", query, headers)

    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    print("Set action response: {}".format(remote_file))
    return 0


def error_for_next_action_response(errorCode):
    conn = _create_https_connection()
    conn.request("POST", "/controller/core/response", str(errorCode))



def main(argv):
    cmd = argv[1]

    if len(argv) > 2:
        filename = argv[2]
    else:
        filename = None

    if cmd == "setMcsPolicy":
        return setMcsPolicyInFakeCloud(filename)
    elif cmd == "getMcsPolicy":
        return getMcsPolicyInFakeCloud(filename)
    elif cmd == "setAlcPolicy":
        return setAlcPolicyInFakeCloud(filename)
    elif cmd == "setSavPolicy":
        return setSavPolicyInFakeCloud(filename)
    elif cmd == "getUserAgent":
        return sendCmdToFakeCloud("controller/userAgent",filename)
    else:
        return sendCmdToFakeCloud(cmd, filename)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
