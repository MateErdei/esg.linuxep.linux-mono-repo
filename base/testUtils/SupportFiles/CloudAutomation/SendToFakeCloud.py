# -*- coding: utf-8 -*-
import http.client
import ssl
import sys



# filename - XML file to set as policy in fake cloud
# policyType - The type of policy, specidifed by ID e.g. "SAV", "MCS", "ALC"
def setPolicyInFakeCloud(filename, policyType):
    with open(filename, "rb") as f:
        data = f.read()

    headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

    # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
    conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
    conn.request("PUT", "/controller/" + policyType + "/policy", data, headers)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    return 0

def getPolicyInFakeCloud(filename, policyType):
    headers = {"Content-type": "application/octet-stream", "Accept": "text/plain"}

    # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
    conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
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
    conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
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

    # Open HTTPS connection to fake cloud at https://127.0.0.1:4443
    conn = http.client.HTTPSConnection("127.0.0.1","4443", context=ssl._create_unverified_context())
    conn.request("PUT", "/controller/livequery/command", query, headers)
    response = conn.getresponse()
    remote_file = response.read()
    conn.close()
    print("Set livequery response: {}".format(remote_file))
    return 0


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
