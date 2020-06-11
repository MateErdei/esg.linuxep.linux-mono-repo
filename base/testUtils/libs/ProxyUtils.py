#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import sys
import subprocess
import os

import PathManager

SUPPORTFILEPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILEPATH)

import IPAddress

from robot.api import logger

def getIP():
    for ip in IPAddress.getNonLocalIPv4():
        return ip

def Calculate_Proxy_IP_at_distance(distance=1):
    """Calculate an IP address that is a fixed 'distance' from the first non-local IPv4 address

Implements code similar to this bash script from the SAV regression suite:
getIfaceInfo "$iface"
getIpAddrFromIfaceInfo "$IFACE_INFO"

IFS='.' read -ra ADDR <<< "$IPADDR"

DIST_25="$(( ADDR[0] ^ 1 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}" #Distance 25
DIST_5="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 16 ))" #Distance 5
DIST_1="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 1 ))" #Distance 1
DIST_9="${ADDR[0]}.${ADDR[1]}.$(( ADDR[2] ^ 1)).${ADDR[3]}" #Distance 9

    :return:
    """
    distance = int(distance)
    assert(distance <= 32)

    ip = getIP()
    if distance == 0:
        return ip


    bytes = ip.split(".")
    byte = 3 - ((distance-1) // 8)

    value = int(bytes[byte])
    bit = (distance%8)-1
    if bit < 0:
        bit += 8
    value ^= 1 << (bit)
    bytes[byte] = str(value)

    return ".".join(bytes)


def modify_hosts_file(**newLines):
    keys = newLines.keys()
    lines = open("/etc/hosts").readlines()
    if not os.path.isfile("/etc/hosts.bak"):
        open("/etc/hosts.bak","w").writelines(lines)
    output = []
    for line in lines:
        keep = True
        for key in keys:
            if key in line:
                logger.info("Found %s in %s"%(key, line))
                keep = False

        if keep:
            output.append(line)

    logger.info("Kept /etc/hosts to: %s"%str(output))

    for key in keys:
        output.append("%s %s\n"%(newLines[key],key))

    logger.info("Output /etc/hosts to: %s"%str(output))

    open("/etc/hosts","w").writelines(output)
    lines = open("/etc/hosts").readlines()
    logger.info("Modified /etc/hosts to: %s"%str(lines))


def unmodify_hosts_file(**newLines):
    keys = newLines.keys()
    lines = open("/etc/hosts").readlines()
    output = []
    for line in lines:
        keep = True
        for key in keys:
            if key in line:
                keep = False
                break
        if keep:
            output.append(line)
    open("/etc/hosts", "w").writelines(output)

def restart_Secure_Server_Proxy():

    SUPPORTFILEPATH = PathManager.get_support_file_path()
    APPSERVER_KEYFILE=os.path.join(SUPPORTFILEPATH, "secureServerProxy", "secure_server_ssh_key")
    assert(os.path.isfile(APPSERVER_KEYFILE))
    os.chmod(APPSERVER_KEYFILE, 0o600)

    command = ["ssh",
               "-o", "StrictHostKeyChecking=no",
               "-i", APPSERVER_KEYFILE,
               ]

    command.append("root@ssplsecureproxyserver.eng.sophos")
    command.append("/etc/init.d/tinyproxy restart")


    output = subprocess.check_output(command)

    return output

def main(argv):
    distance = "5"
    expected = None
    if len(argv) > 1:
        distance = argv[1]

    print("%s:" % distance)

    if len(argv) > 2:
        expected = argv[2]
        print(expected)

    actual = Calculate_Proxy_IP_at_distance(distance)
    print(actual)
    if expected is not None and actual != expected:
        print("***ERRROR %s != %s" % (actual, expected))
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
