# -*- coding: utf-8 -*-
# Copyright 2018-2020 Sophos Plc, Oxford, England.


"""
TargetSystem Module
"""
# pylint: disable=too-many-lines

import json
import platform
import os
import re
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request

from .utils.byte2utf8 import to_utf8

DISTRIBUTION_NAME_MAP = {
    'redhat': 'redhat',
    'ubuntu': 'ubuntu',
    'centos': 'centos',
    'amazonlinux': 'amazon'
}

#-----------------------------------------------------------------------------
# class TargetSystem
#-----------------------------------------------------------------------------


class TargetSystem:
    """
    Represents the system into which we are trying to install.
    """
    # pylint: disable=too-many-instance-attributes, no-self-use
    # pylint: disable=too-many-public-methods, too-many-return-statements

    def __init__(self, install_dir="."):
        """
        __init__
        """
        self._save_uname()
        self.m_install_dir = install_dir
        self.m_description, self.m_distributor_id, self.m_release = self._collect_lsb_release()
        self.m_vendor = self.get_vendor()
        self.m_product = self.m_description
        self.m_os_version = self.get_os_version()

    def _read_uname(self):
        """
        _read_uname
        :return:
        """
        return [platform.node(), platform.release(), platform.machine()]

    def _save_uname(self):
        """
        _save_uname
        :return:
        """
        self.uname = self._read_uname()
        self.last_uname_save = time.time()

    def run_command(self, command, shell=False):
        """
        run_command
        :return:
        """
        # Clean environment
        env = os.environ.copy()
        try:
            proc = subprocess.Popen(command, env=env,
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    shell=shell)
            stdout_b, stderr_b = proc.communicate()  # pylint: disable=unused-variable
            stdout = to_utf8(stdout_b) if stdout_b else ""
            ret_code = proc.wait()
            assert ret_code is not None
        except OSError:
            ret_code = -1
            stdout = ""
        return ret_code, stdout

    def _collect_lsb_release(self):
        """
        Collect any lsb_release info
        """
        # pylint: disable=too-many-locals, too-many-branches, too-many-statements
        description = None
        distributor_id = None
        release = None

        path = os.environ.get("PATH", "")
        lsb_release_exes = []
        for split_path_element in path.split(":"):
            lsb_release_bin = os.path.join(split_path_element, "lsb_release")
            if os.path.isfile(lsb_release_bin):
                lsb_release_exes.append(lsb_release_bin)

        if "/usr/bin/lsb_release" not in lsb_release_exes \
                and os.path.isfile("/usr/bin/lsb_release"):
            lsb_release_exes.append("/usr/bin/lsb_release")

        ret_code = -2
        for lsb_release_exe in lsb_release_exes:
            (ret_code, stdout) = self.run_command([lsb_release_exe, '-a'])
            if ret_code == 0:
                break

        if ret_code == 0:
            lines = stdout.split("\n")
            # Each line consists of "Key:<whitespace>*<value>"
            matcher = re.compile(r"([^:]+):\s*(.+)$")
            for line in lines:
                line_matched = matcher.match(line)
                if line_matched:
                    if "Description" in line:
                        description = line_matched.group(2)
                    elif "Distributor ID" in line:
                        distributor_id = line_matched.group(2)
                    elif "Release" in line:
                        release = line_matched.group(2)

        return description, distributor_id, release

    def get_vendor(self):
        if self.m_distributor_id:
            vendor = self.m_distributor_id.lower().replace(" ", "").replace("/", "_")
            for key in DISTRIBUTION_NAME_MAP:
                if vendor.startswith(key):
                    vendor = DISTRIBUTION_NAME_MAP[key]
                    break
            return vendor
        return None

    def get_os_version(self):
        # Parse description for ubuntu version
        if self.m_vendor == "ubuntu":
            desc = self.m_description
            if desc:
                matched = re.match(r"Ubuntu (\d+)\.(\d+)\.(\d+)", desc)
                if matched:
                    return matched.group(1), matched.group(2), matched.group(3)
                matched = re.match(r"Ubuntu (\d+)\.(\d+)", desc)
                if matched:
                    return matched.group(1), matched.group(2), ""

        # Attempt to parse the Release field into digit blocks
        release = self.m_release
        if release:
            os_version = release.split(".")
            if len(os_version) > 3:
                os_version = os_version[:3]
            else:
                while len(os_version) < 3:
                    os_version.append("")
            return os_version

        number_block_re = re.compile(r"\d+")
        if self.m_product:
            blocks = number_block_re.findall(self.m_product)
            blocks_length = len(blocks)
            if blocks_length > 0:
                return blocks

        # Final fall-back - use kernel version
        kernel = self.kernel()
        if kernel:
            return kernel.strip().split(".")

        return "", "", ""

    def detect_distro(self):
        """
        Detect the name of distro.
        """
        distro_check_files = ['/etc/lsb-release',
                              '/etc/issue',
                              '/etc/centos-release',
                              '/etc/redhat-release',
                              '/etc/system-release']

        distro_identified = None
        for distro_file in distro_check_files:
            if distro_identified:
                break
            distro_identified = self.check_distro_file(
                distro_file)

        if not distro_identified:
            distro_identified = 'unknown'
            self.m_product = 'unknown'

        return distro_identified

    def check_distro_file(self, distro_file):
        """
        Check a file to see if it describes the distro
        """
        if not os.path.exists(distro_file):
            return None
        with open(distro_file) as check_file:
            original_contents = check_file.readline()

        check_contents = original_contents.lower().replace(" ", "")

        # conversion mapping between string at start of file and distro name
        for distro_string in DISTRIBUTION_NAME_MAP:
            if check_contents.startswith(distro_string):
                self.m_product = original_contents.strip()
                return DISTRIBUTION_NAME_MAP[distro_string]
        return None

    def kernel(self):
        """
        Detect the kernel we are running on.
        """
        return self.uname[1]

    def detect_is_ec2_instance(self):
        """
        detect_is_ec2_instance
        :return:
        """
        if os.path.isfile(os.path.join(self.m_install_dir, "etc", "amazon")):
            return True

        try:
            with open('/sys/hypervisor/uuid') as uuid_file:
                if "ec2" in uuid_file.read():
                    return True
        except EnvironmentError:
            pass

        try:
            with open("/sys/devices/virtual/dmi/id/bios_version") as reader:
                if "amazon" in reader.read():
                    return True
        except EnvironmentError:
            pass

        try:
            with open("/sys/devices/virtual/dmi/id/product_uuid") as reader:
                data = reader.read()

            if data.startswith("EC2"):
                return True
        except EnvironmentError:
            pass

        try:
            with open("/var/log/dmesg") as reader:
                results = reader.read()

            results_list = results.split("\n")
            for item in results_list:
                line = item.lower()
                if "bios" in line and "amazon" in line:
                    return True
        except EnvironmentError:
            pass

        try:
            # Don't try two dmidecodes if the first gives an error
            with open("/dev/null", "wb") as devnull:
                results1 = to_utf8(subprocess.check_output(
                    args=["dmidecode", "--string", "system-uuid"],
                    stderr=devnull))
                results2 = to_utf8(subprocess.check_output(
                    args=["dmidecode", "-s", "bios-version"], stderr=devnull))
                if results1.startswith("EC2") or "amazon" in results2:
                    return True
        except (subprocess.CalledProcessError, EnvironmentError):
            pass

        return False

    def detect_instance_info(self):
        """
        detect_instance_info
        :return:
        """

        if self.detect_is_ec2_instance():
            try:
                proxy_handler = urllib.request.ProxyHandler({})
                opener = urllib.request.build_opener(proxy_handler)
                aws_info_string = opener.open(
                    "http://169.254.169.254/latest/dynamic/instance-identity/document",
                    timeout=1).read()
                aws_info = json.loads(aws_info_string)
                return {"region": aws_info["region"],
                        "accountId": aws_info["accountId"],
                        "instanceId": aws_info["instanceId"]}
            except urllib.error.URLError:
                pass
        return None

    def architecture(self):
        """
        Detect on which architecture we are running on.
        """
        return self.uname[2]

    def hostname(self):
        """
        hostname
        :return:
        """
        hostname = self.uname[0].split(".")[0]
        ten_mins = 600
        if (abs(time.time() - self.last_uname_save)
                > ten_mins or hostname == "localhost"):
            self._save_uname()
            hostname = self.uname[0].split(".")[0]
        return hostname

    def vendor(self):
        """
        vendor
        :return:
        """
        if self.m_vendor:
            return self.m_vendor
        return self.detect_distro()

    def os_version(self):
        """
        os_version
        :return:
        """
        return self.m_os_version

    def os_name(self):
        """
        os_name
        :return:
        """
        return self.m_product
