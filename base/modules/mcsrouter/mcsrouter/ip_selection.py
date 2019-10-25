#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
ip_selection Module
"""

import ipaddress
import logging
import socket
import threading
import time

from . import ip_address

LOGGER = logging.getLogger(__name__)


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
        attempt = 0
        try:
            while attempt < 2:
                attempt += 1
                try:
                    addr_info = socket.getaddrinfo(self.server["hostname"], None)
                    LOGGER.debug('AddrInfo: {}'.format(addr_info))
                    ip_set = set([i[4][0] for i in addr_info])
                    self.server["ips"] = list(ip_set)
                # some times the following error occurs:
                # File "socket.py", line 748, in getaddrinfo
                #   for res in _socket.getaddrinfo(host, port, family, type, proto, flags):
                # LookupError: unknown encoding: idna
                # But it seems not to persist. Try again.
                except LookupError as ex:
                    LOGGER.info('Failed in extracting  hostname info {}'.format(ex))
                    time.sleep(0.1)
                    if attempt == 2:
                        raise
        except Exception as ex:  # pylint: disable=broad-except
            msg = "Extracting ip from server {} resulted in exception {}"
            LOGGER.debug(msg.format(self.server['hostname'], ex))


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
        return int(ipaddress.IPv4Address(ip_addr))
    return int(ipaddress.IPv6Address(ip_addr))


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
    servers = server_location_list.copy()
    max_lookup_timeout = 20

    # Start an address resolution thread for each hostname
    lookup_threads = []
    for server in servers:
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

def order_message_relays(server_location_list, local_ipv4s, local_ipv6s):
    """
    Order the message relays based on ip distance and priority.
    :param server_location_list: list of dictionary entries containing at least: hostname,
                                 priority and ips
    :param local_ipv4s: list of ip4 strings. eg: ['10.0.1.2']
    :param local_ipv6s:
    :return: copy of server_location_list sorted by ip distance and priority.
    @see test_alg_evaluate_address_preference
    """
    server_location_list = order_by_ip_address_distance(
        local_ipv4s, local_ipv6s, server_location_list)
    LOGGER.debug("Ordered by distance: {}".format(server_location_list))
    # Order by priority
    server_location_list = order_servers_by_key(
        server_location_list, "priority")
    LOGGER.debug("Ordered by priority: {}".format(server_location_list))
    return server_location_list



def evaluate_address_preference(server_location_list):
    """
    Function takes a list of dictionaries with contents {hostname: <hostname>, priority: <priority>}
    and sorts them in terms of bit-wise similarity of IP address, and then priority
    """
    if len(server_location_list) < 2:
        return server_location_list
    LOGGER.debug("Evaluate Preference server location list: {}".format(server_location_list))
    # Order by IP address
    local_ipv4s = list(ip_address.get_non_local_ipv4())
    LOGGER.debug("IPV4: {}".format(local_ipv4s))
    local_ipv6s = [int(ipv6, 16) for ipv6 in ip_address.get_non_local_ipv6()]
    LOGGER.debug("IPV6: {}".format(local_ipv6s))
    server_location_list = get_server_ips_from_hostname(server_location_list)
    LOGGER.debug("Evaluate Preference server location list with ips: {}"
                 .format(server_location_list))
    return order_message_relays(server_location_list, local_ipv4s, local_ipv6s)
