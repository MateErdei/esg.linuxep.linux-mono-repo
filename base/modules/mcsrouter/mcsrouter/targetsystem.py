# -*- coding: utf-8 -*-
# Copyright 2020 Sophos Plc, Oxford, England.


"""
TargetSystem Module
"""
# pylint: disable=too-many-lines

import glob
import json
import os
import re
import socket
import subprocess
import sys
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

        # First check if lsb_release is available
        self.m_description, self.m_distributor_id, self.m_release = self.__collect_lsb_release()

        self.m_vendor = self.get_vendor()
        self.product = self.get_product()
        self.m_os_version = self.get_os_version()
        self.ambiguous = None
        self.os_name = self.product

    def _read_uname(self):
        """
        _read_uname
        :return:
        """
        try:
            return os.uname()
        except OSError:  # os.uname() can fail on HP-UX with > 8 character hostnames
            return socket.gethostname()

    def _save_uname(self):
        """
        _save_uname
        :return:
        """
        self.uname = self._read_uname()
        self.last_uname_save = time.time()

    def __back_tick(self, command):
        """
        __back_tick
        :return:
        """
        if isinstance(command, str):
            command = [command]
        # Clean environment
        env = os.environ.copy()

        # Need to reset the python variables in case command is a python script
        env_variables = ['PYTHONPATH', 'LD_LIBRARY_PATH', 'PYTHONHOME']
        for variable in env_variables:
            orig = "ORIGINAL_%s" % variable
            if orig in env and env[orig] != "":
                env[variable] = env[orig]
            else:
                env.pop(variable, None)  # Ignore the variable if it's missing

        def attempt(command, env, shell=False):
            """
            attempt
            :return:
            """
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

        ret_code, stdout = attempt(command, env)
        if ret_code == 0:
            return ret_code, stdout

        # Maybe we tried to execute a Python program with incorrect environment
        # so lets try again without any.
        clear_env = env.copy()
        for variable in env_variables:
            clear_env.pop(variable, None)

        ret_code, stdout = attempt(command, clear_env)
        if ret_code == 0:
            return ret_code, stdout

        # Try via the shell

        ret_code, stdout = attempt(command, env, shell=True)
        if ret_code == 0:
            return ret_code, stdout
        ret_code, stdout = attempt(command, clear_env, shell=True)
        if ret_code == 0:
            return ret_code, stdout

        # Try with restricted PATH
        env['PATH'] = "/usr/bin:/bin:/usr/sbin:/sbin"
        ret_code, stdout = attempt(command, env)
        if ret_code == 0:
            return ret_code, stdout
        clear_env['PATH'] = env['PATH']
        ret_code, stdout = attempt(command, clear_env)
        if ret_code == 0:
            return ret_code, stdout

        return ret_code, stdout

    def __collect_lsb_release(self):
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
            (ret_code, stdout) = self.__back_tick([lsb_release_exe, '-a'])
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
        # Correct distribution
        if self.m_distributor_id:
            # Lower case and strip spaces
            vendor = self.m_distributor_id.lower()
            vendor = vendor.replace(" ", "")
            vendor = vendor.replace("/", "_")
            match = False
            for key in DISTRIBUTION_NAME_MAP:
                if vendor.startswith(key):
                    vendor = DISTRIBUTION_NAME_MAP[key]
                    match = True
                    break
            if not match:
                if vendor in ['n_a', '']:
                    vendor = self.m_lsb_description
                    vendor = vendor.lower()
                    vendor = vendor.replace(" ", "")
                    vendor = vendor.replace("/", "_")
                    for key in DISTRIBUTION_NAME_MAP:
                        if vendor.startswith(key):
                            vendor = DISTRIBUTION_NAME_MAP[key]
                            match = True
                            break
                if not match:
                    # Create a valid distribution name from whatever we're given
                    # Endswith
                    length = len(vendor) - len("linux")
                    if length > 0 and vendor.rfind("linux") == length:
                        vendor = vendor[:length]
            return vendor
        return None

    def get_product(self):
        return self.m_description

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
            os_version = []
            while True:
                matched = re.match(r"(\d+)\.(.*)", release)
                if matched:
                    os_version.append(matched.group(1))
                    release = matched.group(2)
                    continue
                break
            while len(os_version) < 3:
                os_version.append("")
            return os_version

        number_block_re = re.compile(r"\d+")
        if self.product:
            blocks = number_block_re.findall(self.product)
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
                              '/etc/UnitedLinux-release',
                              '/etc/system-release']

        distro_identified = None
        for distro_file in distro_check_files:
            distro_identified = self.check_distro_file(
                distro_file, distro_identified)

        if distro_identified is None:
            distro_identified = 'unknown'
            self.product = 'unknown'

        return distro_identified

    def check_distro_file(self, distro_file, distro_identified):
        """
        Check a file to see if it describes the distro
        """
        if not os.path.exists(distro_file):
            return distro_identified
        check_file = open(distro_file)
        original_contents = check_file.readline().strip()
        check_file.close()
        # Lowercase and strip spaces to match map table content
        check_contents = original_contents.lower()
        check_contents = check_contents.replace(" ", "")

        # conversion mapping between string at start of file and distro name

        for distro_string in DISTRIBUTION_NAME_MAP:
            if check_contents.startswith(distro_string):
                if distro_identified:
                    if distro_identified != DISTRIBUTION_NAME_MAP[distro_string]:
                        # Ambiguous distro!
                        if self.ambiguous is None:
                            self.ambiguous = [distro_identified]
                        self.ambiguous = self.ambiguous + \
                            [DISTRIBUTION_NAME_MAP[distro_string]]
                        distro_identified = 'unknown'
                        self.product = 'unknown'
                else:
                    distro_identified = DISTRIBUTION_NAME_MAP[distro_string]
                    self.product = original_contents
        return distro_identified

    def kernel(self):
        """
        Detect the kernel we are running on.
        """
        return self.uname[2]

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
        return self.uname[4]

    def hostname(self):
        """
        hostname
        :return:
        """
        hostname = self.uname[1].split(".")[0]
        ten_mins = 600
        if (abs(time.time() - self.last_uname_save)
                > ten_mins or hostname == "localhost"):
            self._save_uname()
            hostname = self.uname[1].split(".")[0]
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
