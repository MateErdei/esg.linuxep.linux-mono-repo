#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2013-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys
import socket
import fcntl
import struct
import array
import subprocess

"""
Get IP Addresses for the current system.

NB: This is Linux specific at the moment.

See test/sophosmgmtd/getip.py for some alternatives
"""

def all_interfaces():
    is_64bits = sys.maxsize > 2**32
    struct_size = 40 if is_64bits else 32
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    max_possible = 8  # initial value
    while True:
        bytes = max_possible * struct_size
        names = array.array('B', b'\0' * bytes)
        outbytes = struct.unpack(b'iL', fcntl.ioctl(
            s.fileno(),
            0x8912,   # SIOCGIFCONF
            struct.pack(b'iL', bytes, names.buffer_info()[0])
        ))[0]
        if outbytes == bytes:
            max_possible *= 2
        else:
            break
    namestr = names.tostring()
    return [(namestr[i:i+16].split(b'\0', 1)[0],
             socket.inet_ntoa(namestr[i+20:i+24]))
            for i in range(0, outbytes, struct_size)]

def getNonLocalIPv4():
    """
    Generates all IPv4 addresses, that aren't in the 127.* range
    Starts with the best (eth, em) and gets worse (vmware etc).
    """
    allinterfaces = all_interfaces()
    if len(allinterfaces) == 0:
        return

    ip_ordering = []
    for (i,ip) in allinterfaces:
        if ip.startswith("127."):
            pass
        elif i.startswith(b"eth"):
            ip_ordering.append((0,i,ip))
        elif i.startswith(b"em"):
            ip_ordering.append((1,i,ip))
        elif i.startswith(b"docker"):
            ip_ordering.append((200,i,ip))
        elif ip.startswith("172."):
            ip_ordering.append((150,i,ip))
        else:
            ip_ordering.append((100,i,ip))

    seen = {}

    ip_ordering.sort()
    for (_,interface,ip) in ip_ordering:
        if seen.get(ip,0) == 1:
            continue
        seen[ip] = 1
        yield ip

def getNonLocalIPv6():
    """
    Generates all IPv6 addresses, that aren't in the 127.* range
    Starts with the best (eth0) and gets worse (vmware etc).

    """
    try:
        data = open(b"/proc/net/if_inet6").read()
    except EnvironmentError:
        return

    lines = data.splitlines()
    m = {}
    for line in lines:
        line = line.strip()
        fields = line.split()
        m[fields[-1]] = fields[0]

    ethips = []
    otherips = []
    for (d,ip) in m.iteritems():
        if ip == b"00000000000000000000000000000001":
            continue
        elif d.startswith(b"eth"):
            ethips.append((d,ip))
        else:
            otherips.append((d,ip))

    seen = {}

    ethips.sort()
    for (device,ip) in ethips:
        if seen.get(ip,0) == 1:
            continue
        seen[ip] = 1
        yield ip

    otherips.sort()
    for (device,ip) in otherips:
        if seen.get(ip,0) == 1:
            continue
        seen[ip] = 1
        yield ip


def getFQDN():
    s = socket.getfqdn()
    if b"." in s:
        return s

    devnull = open(os.devnull, b"wb")
    try:
        output = subprocess.check_output([b'hostname', b'-f'], stderr=devnull)
    except OSError:
        ## hostname doesn't exist - got nothing better than s
        return s
    except subprocess.CalledProcessError:
        ## hostname -f returned an error - got nothing better than s
        return s
    finally:
        devnull.close()

    return output.strip()
