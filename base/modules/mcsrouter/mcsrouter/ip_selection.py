#!/usr/bin/env python
"""
ip_selection Module
"""

import socket
import threading

from . import ip_address


class IpLookupThread(threading.Thread):
    """
    IpLookupThread
    """

    def __init__(self, server):
        """
        __init__
        """
        super(IpLookupThread, self).__init__()
        self.server = server

    def run(self):
        """
        run
        """
        try:
            self.server["ips"] = list(
                set([i[4][0] for i in socket.getaddrinfo(self.server["hostname"], None)]))
        except socket.gaierror:
            pass


def order_servers_by_key(server_location_list, key_string):
    """
    order_servers_by_key
    """
    server_location_list = sorted(
        server_location_list,
        key=lambda k: k[key_string],
        reverse=False)
    return server_location_list


def ip_string_to_int(ip_addr, ip_type):
    """
    ip_string_to_int
    """
    if ip_type == "ipv4":
        return int(socket.inet_pton(socket.AF_INET, ip_addr).encode('hex'), 16)
    return int(socket.inet_pton(socket.AF_INET6, ip_addr).encode('hex'), 16)


def get_ip_address_distance(local_ip, remote_ip, ip_type="ipv4"):
    """
    get_ip_address_distance
    """
    if isinstance(local_ip, str):
        local_ip_int = ip_string_to_int(local_ip, ip_type)
    else:
        local_ip_int = local_ip
    remote_ip_int = ip_string_to_int(remote_ip, ip_type)
    return (local_ip_int ^ remote_ip_int).bit_length()


def get_server_ips_from_hostname(server_location_list):
    """
    get_server_ips_from_hostname
    """
    max_lookup_timeout = 20

    # Start an address resolution thread for each hostname
    lookup_threads = []
    for server in server_location_list:
        lookup_threads.append(IpLookupThread(server))

    for thread in lookup_threads:
        thread.daemon = True
        thread.start()

    for thread in lookup_threads:
        thread.join(max_lookup_timeout)

    return [domain for domain in server_location_list if 'ips' in domain]


def order_by_ip_address_distance(
        local_ipv4s,
        local_ipv6s,
        server_location_list):
    """
    order_by_ip_address_distance
    """
    for server in server_location_list:
        min_dist = 1000  # Initialise min_dist with a big number

        for remote_ip in server["ips"]:
            # If ip contains a dot, it's ipv4
            if "." in remote_ip:
                for local_ip in local_ipv4s:
                    dist = get_ip_address_distance(local_ip, remote_ip)
                    if dist < min_dist:
                        min_dist = dist
            else:
                for local_ip in local_ipv6s:
                    dist = get_ip_address_distance(local_ip, remote_ip, "ipv6")
                    if dist < min_dist:
                        min_dist = dist

        server['dist'] = min_dist
    server_location_list = order_servers_by_key(server_location_list, 'dist')
    return server_location_list


def evaluate_address_preference(server_location_list):
    """
    Function takes a list of dictionaries with contents {hostname: <hostname>, priority: <priority>}
    and sorts them in terms of bit-wise similarity of IP address, and then priority
    """
    if len(server_location_list) < 2:
        return server_location_list

    # Order by IP address
    local_ipv4s = list(ip_address.get_non_local_ipv4())
    local_ipv6s = [int(ipv6, 16) for ipv6 in ip_address.get_non_local_ipv6()]
    server_location_list = get_server_ips_from_hostname(server_location_list)
    server_location_list = order_by_ip_address_distance(
        local_ipv4s, local_ipv6s, server_location_list)

    # Order by priority
    server_location_list = order_servers_by_key(
        server_location_list, "priority")

    return server_location_list
