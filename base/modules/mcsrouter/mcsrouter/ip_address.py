#!/usr/bin/env python
"""
Get IP Addresses for the current system.

NB: This is Linux specific at the moment.

See test/mcsrouter/getip.py for some alternatives
"""

import os
import sys
import socket
import fcntl
import struct
import array
import subprocess


def get_all_interfaces():
    """
    get_all_interfaces
    """
    is_64bits = sys.maxsize > 2**32
    struct_size = 40 if is_64bits else 32
    socket_instance = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    max_possible = 8  # initial value
    while True:
        number_of_bytes = max_possible * struct_size
        names = array.array('B', b'\0' * number_of_bytes)
        out_bytes = struct.unpack('iL', fcntl.ioctl(
            socket_instance.fileno(),
            0x8912,  # SIOCGIFCONF
            struct.pack('iL', number_of_bytes, names.buffer_info()[0])
        ))[0]
        if out_bytes == number_of_bytes:
            max_possible *= 2
        else:
            break
    name_string = names.tobytes()
    return [(name_string[i:i + 16].split(b'\0', 1)[0],
             socket.inet_ntoa(name_string[i + 20:i + 24]))
            for i in range(0, out_bytes, struct_size)]


def get_non_local_ipv4():
    """
    Generates all IPv4 addresses, that aren't in the 127.* range
    Starts with the best (eth, em) and gets worse (vmware etc).
    """
    all_interfaces = get_all_interfaces()
    if not all_interfaces:
        return

    ip_ordering = []
    for (index, ip_address) in all_interfaces:
        if ip_address.startswith("127."):
            pass
        elif index.startswith(b"eth"):
            ip_ordering.append((0, index, ip_address))
        elif index.startswith(b"em"):
            ip_ordering.append((1, index, ip_address))
        elif index.startswith(b"docker"):
            ip_ordering.append((200, index, ip_address))
        elif ip_address.startswith("172."):
            ip_ordering.append((150, index, ip_address))
        else:
            ip_ordering.append((100, index, ip_address))

    seen = {}

    ip_ordering.sort()
    for (_, interface, ip_address) in ip_ordering:  # pylint: disable=unused-variable
        if seen.get(ip_address, 0) == 1:
            continue
        seen[ip_address] = 1
        yield ip_address


def get_non_local_ipv6():
    """
    Generates all IPv6 addresses, that aren't in the 127.* range
    Starts with the best (eth0) and gets worse (vmware etc).

    """
    try:
        with open("/proc/net/if_inet6") as reader:
            data = reader.read()

    except EnvironmentError:
        return

    lines = data.splitlines()
    field = {}
    for line in lines:
        line = line.strip()
        fields = line.split()
        field[fields[-1]] = fields[0]

    ethernet_ips = []
    other_ips = []
    for (device, ip_address) in field.items():
        if ip_address == "00000000000000000000000000000001":
            continue
        elif device.startswith("eth"):
            ethernet_ips.append((device, ip_address))
        else:
            other_ips.append((device, ip_address))

    seen = {}

    ethernet_ips.sort()
    for (device, ip_address) in ethernet_ips:
        if seen.get(ip_address, 0) == 1:
            continue
        seen[ip_address] = 1
        yield ip_address

    other_ips.sort()
    for (device, ip_address) in other_ips:
        if seen.get(ip_address, 0) == 1:
            continue
        seen[ip_address] = 1
        yield ip_address


def get_fqdn():
    """
    get_fqdn
    """
    fqdn_socket = socket.getfqdn()
    if "." in fqdn_socket:
        return fqdn_socket

    dev_null = open(os.devnull, "wb")
    try:
        output = subprocess.check_output(['hostname', '-f'], stderr=dev_null)
    except OSError:
        # hostname doesn't exist - got nothing better than s
        return fqdn_socket
    except subprocess.CalledProcessError:
        # hostname -f returned an error - got nothing better than s
        return fqdn_socket
    finally:
        dev_null.close()

    return output.strip()
