#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.


# Utilities for managing the hosts file

import os


def revert_hostfile():
    if os.path.isfile(b"/etc/hosts.bak"):
        os.rename(b"/etc/hosts", b"/etc/hosts.tmp")
        os.rename(b"/etc/hosts.bak", b"/etc/hosts")
        os.remove(b"/etc/hosts.tmp")


def append_host_file(ip_address, hostname):
    """
    Appends the hostfile. It is important the file is reverted at the end of a test where this function is used.
    :param ip_address: string of the ip address
    :param hostname: string of the hostname
    """
    lines = open(b"/etc/hosts").readlines()
    if not os.path.isfile(b"/etc/hosts.bak"):
        open(b"/etc/hosts.bak", "w").writelines(lines)
    line = "{} {}\n".format(ip_address, hostname)
    if line not in lines:
        lines.append(line)
    open(b"/etc/hosts", "w").writelines(lines)
