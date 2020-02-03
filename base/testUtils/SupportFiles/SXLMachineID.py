#! /usr/bin/env python
# -*- coding: utf-8 -*-
# copied from //dev/savlinux/trunk/products/CommandLine/util
## Ideas from http://code.activestate.com/recipes/577191-ip-and-mac-addresses/

import subprocess
import os
import hashlib
import re
import targetsystem as tsystem

SXL_MACHINE_ID_FILENAME = "machine_ID.txt"

IFCONFIG_HWADDR_RE = re.compile(r"HWaddr ([\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2})")
IFCONFIG_ETHER_RE = re.compile(r"ether ([\dA-Fa-f]{1,2}:[\dA-Fa-f]{1,2}:[\dA-Fa-f]{1,2}:[\dA-Fa-f]{1,2}:[\dA-Fa-f]{1,2}:[\dA-Fa-f]{1,2})")
IFCONFIG_INET_IP_RE = re.compile(r"inet (?:addr:|)(\d+.\d+.\d+.\d+)")
ARP_RE = re.compile(r"\? (\d+.\d+.\d+.\d+) at ([\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2})")
ARP2_RE = re.compile(r"\s(\d+.\d+.\d+.\d+)\s+255\.255\.255\.255\s+SP\s+([\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2})")
NETSTAT_RE = re.compile(r"Hardware Address: ([\da-f]{2}:[\da-f]{2}:[\da-f]{2}:[\da-f]{2}:[\da-f]{2}:[\da-f]{2})")
IP_HWADDR_RE = re.compile(r"link/ether ([\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2}:[\dA-Fa-f]{2})")

def _findCommand(base):
    c = os.path.join("/sbin", base)
    if os.path.isfile(c):
        return c
    c = os.path.join("/usr/sbin", base)
    if os.path.isfile(c):
        return c
    c = os.path.join("/bin", base)
    if os.path.isfile(c):
        return c
    c = os.path.join("/usr/bin", base)
    if os.path.isfile(c):
        return c
    return None

def popen(command):
    env = os.environ.copy()
    env['LC_ALL'] = "C"
    env['LANG'] = "C"
    return subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=env)

def _generateIpLines(option="addr"):
    ipcmd = _findCommand("ip")
    if ipcmd is None:
        return

    command = [ipcmd, option]

    proc = popen(command)
    output = proc.communicate()[0].decode('utf-8')
    code = proc.wait()
    if code != 0:
        return

    for line in output.splitlines():
        yield line

def _generateMacsIp(targetsystem):
    for line in _generateIpLines():
        mo = IP_HWADDR_RE.search(line)
        if mo:
            yield mo.group(1)

def _generateIfconfigLines(option="-a"):
    """
    ifconfig -a generates MAC addresses on Linux and a few others,
    but we want to ignore the errors from ifconfig -a on HPUX
    """
    ifconfig = _findCommand("ifconfig")
    if ifconfig is None:
        return

    command = [ifconfig, option]
    proc = popen(command)
    output = proc.communicate()[0].decode('utf-8')
    code = proc.wait()
    if code != 0:
        return

    for line in output.splitlines():
        yield line

def _generateIps(targetsystem):
    for line in _generateIfconfigLines():
        mo = IFCONFIG_INET_IP_RE.search(line)
        if mo:
            yield mo.group(1)


def _generateMacsArp(targetsystem):
    arp = _findCommand("arp")
    if arp is None:
        return

    command = [arp, '-an']
    proc = popen(command)
    output = proc.communicate()[0].decode('utf-8')
    code = proc.wait()
    if code != 0:
        return

    ips = list(_generateIps(targetsystem))
    for line in output.splitlines():
        mo = ARP_RE.search(line)
        if mo:
            if mo.group(1) in ips:
                yield mo.group(2)
        else:
            mo = ARP2_RE.search(line)
            if mo:
                if mo.group(1) in ips:
                    yield mo.group(2)

def _generateMacsIfconfig(targetsystem=None):
    for line in _generateIfconfigLines():
        mo = IFCONFIG_HWADDR_RE.search(line)
        if mo:
            yield mo.group(1)
        mo = IFCONFIG_ETHER_RE.search(line)
        if mo:
            yield mo.group(1)


def _generateMacsLanscan(targetsystem):
    """
    HPUX - `lanscan -a` prints out the mac address, one per-line
    """
    lanscan = _findCommand("lanscan")
    if lanscan is not None:
        command = [lanscan, '-a']
        proc = popen(command)
        output = proc.communicate()[0].decode('utf-8')
        code = proc.wait()
        if code == 0:
            for line in output.splitlines():
                yield line

def _generateMacsNetstat(targetsystem):
    """
    AIX - generate MACs from netstat -v | grep "Hardware Address"
    """
    netstat = _findCommand("netstat")
    if netstat is None:
        return

    command = [netstat, '-v']
    proc = popen(command)
    output = proc.communicate()[0].decode('utf-8')
    code = proc.wait()
    if code == 0:
        for line in output.splitlines():
            mo = NETSTAT_RE.match(line)
            if mo:
                yield mo.group(1)

def _tidyMac(mac):
    sections = mac.split(":")
    res = []
    for s in sections:
        if len(s) == 1:
            res.append("0%s"%s)
        else:
            res.append(s)
    return ":".join(res)

def _generateMacs(targetsystem):
    for mac in _generateMacsIp(targetsystem):
        yield _tidyMac(mac)

    for mac in _generateMacsIfconfig(targetsystem):
        yield _tidyMac(mac)

    for mac in _generateMacsLanscan(targetsystem):
        yield _tidyMac(mac)

    for mac in _generateMacsArp(targetsystem):
        yield _tidyMac(mac)

    if not targetsystem:
        for mac in _generateMacsNetstat(targetsystem):
            yield _tidyMac(mac)

def generateMachineId(targetsystem=tsystem.TargetSystem()):
    macs = {}
    for mac in _generateMacs(targetsystem):
        macs[mac] = 1

    macs = list(macs.keys())
    macs.sort()
    macHash = hashlib.md5()
    #replace first line from savlinuxunix-machineid
    s = 'sspl-machineid'
    macHash.update("sspl-machineid".encode('utf-8')) ## An extra string so windows won't match on machine id
    for mac in macs:
        s += mac
        macHash.update(mac.encode('utf-8'))

    macHash2 = hashlib.md5(macHash.hexdigest().encode('utf-8'))

    return macHash2.hexdigest()

def writeFile(path, contents, permission):
    oldumask = os.umask(0o177)
    f = open(path, "w")
    os.chmod(path, permission)
    f.write(contents)
    f.close()
    os.umask(oldumask)
    os.chmod(path, permission)

def writeMachineIdIfRequired(instdir, targetsystem):
    """
    @return False if machine ID didn't need to be written
    """
    enginedir = os.path.join(instdir, "engine")
    if not os.path.isdir(enginedir):
        return False
    machineIDFile = os.path.join(enginedir, SXL_MACHINE_ID_FILENAME)
    if os.path.isfile(machineIDFile):
        ## Already generated - regenerate if it is the default value since that
        ## is an error
        existing = open(machineIDFile).read().strip()
        if existing != "07bdc20cee569c7000fdf28d94b7fa65":
            return False

    machineID = generateMachineId(targetsystem)
    writeFile(machineIDFile, machineID, 0o644)

    return True
