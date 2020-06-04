#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.


import subprocess
from robot.api import logger


def get_value_after_prefix_from_command(cmd, prefix):
    output = subprocess.check_output(cmd)
    result = output.decode("utf-8")
    logger.info("cmd: {}, result: {}".format(cmd, result))
    result = result.strip()
    result = result.splitlines()
    for line in result:
        line = line.strip()
        if prefix in line:
            value = line.split(prefix, 1)[-1]
            value = value.strip()
            value = value.lstrip('"')
            value = value.rstrip('"')
            logger.info("value: {}".format(value))
            return value
    return ""


def get_value_from_command(cmd):
    output = subprocess.check_output(cmd)
    result = output.decode("utf-8")
    result = result.rstrip()
    logger.info("cmd: {}, result: {}".format(cmd, result))
    return result


def get_kernel_version():
    return get_value_after_prefix_from_command(["/usr/bin/hostnamectl"], "Kernel:")


def get_os_name():
    return get_value_after_prefix_from_command(["cat", "/etc/os-release"], "NAME=")


def get_os_version():
    return get_value_after_prefix_from_command(["cat", "/etc/os-release"], "VERSION_ID=")


def get_arch():
    return get_value_after_prefix_from_command(["/usr/bin/hostnamectl"], "Architecture:")


def get_number_of_cpu_cores():
    return int(get_value_after_prefix_from_command(["/usr/bin/lscpu"], "CPU(s):"))


def get_memory_total():
    totalRow = get_value_after_prefix_from_command(["/usr/bin/free", "-t"], "Total:")
    return int(totalRow.split()[0])


def get_mounts():
    mounts = []
    cmd = ["/bin/df", "-P", "-T", "--local"]
    output = subprocess.check_output(cmd)
    result = output.decode("utf-8")
    logger.info("cmd: {}, result: {}".format(cmd, result))
    result = result.strip()
    result = result.splitlines()
    result.pop(0)

    for line in result:
        line = line.strip()
        columns = line.split()
        mount = {}
        mount["Filesystem"] = columns[0].strip()
        mount["Type"] = columns[1].strip()
        mount["1024-blocks"] = columns[2].strip()
        mount["Used"] = columns[3].strip()
        mount["Available"] = columns[4].strip()
        mount["Capacity"] = columns[5].strip()
        mount["Mounted"] = columns[6].strip()
        mounts.append(mount)
    logger.info("Mounts: {}".format(mounts))
    return mounts


def get_fstypes_and_free_space():
    mounts = []
    cmd = ["/bin/df", "-P", "-T", "--local"]
    output = subprocess.check_output(cmd)
    result = output.decode("utf-8")
    logger.info("cmd: {}, result: {}".format(cmd, result))
    result = result.strip()
    result = result.splitlines()
    result.pop(0)

    for line in result:
        line = line.strip()
        columns = line.split()
        mount = {}
        mount["fstype"] = columns[1].strip()
        mount["free"] = columns[4].strip()
        mounts.append(mount)
    logger.info("Mounts: {}".format(mounts))
    return mounts


def get_locale():
    return get_value_after_prefix_from_command(["/usr/bin/localectl"], "LANG=")


def get_timezone():
    return get_value_after_prefix_from_command(["/usr/bin/timedatectl"], "Time zone:")


def get_abbreviated_timezone():
    return get_value_from_command(["/bin/date", "+%Z"])


def get_up_time_seconds():
    cmd = ["cat", "/proc/uptime"]
    output = subprocess.check_output(cmd)
    result = output.decode("utf-8")
    result = result.strip()
    result = result.split()[0]
    return int(float(result))


def get_selinux_status():
    return get_value_from_command(["getenforce"])


def get_apparmor_status():
    return get_value_from_command(["systemctl", "is-active", "apparmor"])


def get_auditd_status():
    return get_value_from_command(["systemctl", "is-active", "auditd"])

def is_aws_instance():
    proc = subprocess.Popen(["wget", "http://169.254.169.254/latest/dynamic/instance-identity/document"])
    output, error = proc.communicate()
    returncode = proc.returncode
    if returncode == 0:
        return True
    else:
        return False
