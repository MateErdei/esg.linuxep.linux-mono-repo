#!/usr/bin/env python

import socket
import threading
import IPAddress

class IpLookupThread(threading.Thread):
    def __init__(self, server):
        super(IpLookupThread, self).__init__()
        self.server = server

    def run(self):
        try:
            self.server["ips"] = list(set([i[4][0] for i in socket.getaddrinfo(self.server["hostname"], None)]))
        except socket.gaierror:
            pass

def orderServersByKey(serverLocationList, keyString):
    serverLocationList = sorted(serverLocationList, key=lambda k: k[keyString], reverse=False)
    return serverLocationList

def ipStringToInt(ipAddr, ipType):
    if ipType == "ipv4":
        return int(socket.inet_pton(socket.AF_INET, ipAddr).encode('hex'), 16)
    return int(socket.inet_pton(socket.AF_INET6, ipAddr).encode('hex'), 16)

def getIPAddressDistance(localIp, remoteIp, ipType="ipv4"):
    if isinstance(localIp, str):
        localIpInt = ipStringToInt(localIp, ipType)
    else:
        localIpInt = localIp
    remoteIpInt = ipStringToInt(remoteIp, ipType)
    return (localIpInt ^ remoteIpInt).bit_length()

def getServerIpsFromHostname(serverLocationList):
    maxLookupTimeout = 20

    # Start an address resolution thread for each hostname
    lookupThreads = []
    for server in serverLocationList:
        lookupThreads.append(IpLookupThread(server))

    for thread in lookupThreads:
        thread.daemon = True
        thread.start()

    for thread in lookupThreads:
        thread.join(maxLookupTimeout)

    return [d for d in serverLocationList if 'ips' in d]

def orderServersByIpAddressDistance(localIpv4s, localIpv6s, serverLocationList):
    for server in serverLocationList:
        min_dist = 1000   #Initialise min_dist with a big number

        for remoteIp in server["ips"]:
            # If ip contains a dot, it's ipv4
            if "." in remoteIp:
                for localIp in localIpv4s:
                    dist = getIPAddressDistance(localIp, remoteIp)
                    if dist < min_dist:
                        min_dist = dist
            else:
                for localIp in localIpv6s:
                    dist = getIPAddressDistance(localIp, remoteIp, "ipv6")
                    if dist < min_dist:
                        min_dist = dist

        server['dist'] = min_dist
    serverLocationList = orderServersByKey(serverLocationList, 'dist')
    return serverLocationList

def evaluateAddressPreference(serverLocationList):
    """
    Function takes a list of dictionaries with contents {hostname: <hostname>, priority: <priority>}
    and sorts them in terms of bit-wise similarity of IP address, and then priority
    """
    if len(serverLocationList) < 2:
        return serverLocationList

    #Order by IP address
    localIpv4s = list(IPAddress.getNonLocalIPv4())
    localIpv6s = [int(i, 16) for i in IPAddress.getNonLocalIPv6()]
    serverLocationList = getServerIpsFromHostname(serverLocationList)
    serverLocationList = orderServersByIpAddressDistance(localIpv4s, localIpv6s, serverLocationList)

    #Order by priority
    serverLocationList = orderServersByKey(serverLocationList, "priority")

    return serverLocationList
