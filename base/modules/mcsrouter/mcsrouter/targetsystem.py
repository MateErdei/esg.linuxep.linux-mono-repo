# Copyright 2018-2023 Sophos Limited. All rights reserved.
# -*- coding: utf-8 -*-


"""
TargetSystem Module
"""

import json
import platform
import os
import re
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request
import logging
import stat

from .utils import path_manager
from .utils import filesystem_utils
from .utils import get_ids
from .utils.byte2utf8 import to_utf8

LOGGER = logging.getLogger(__name__)

#-----------------------------------------------------------------------------
# class TargetSystem
#-----------------------------------------------------------------------------


class TargetSystem:
    """
    Represents the system into which we are trying to install.
    """

    def __init__(self, install_dir="."):
        """
        __init__
        """
        self._save_uname()
        self.m_install_dir = install_dir
        self.m_description, self.m_distributor_id, self.m_release = _collect_os_release()
        self.m_vendor = self._get_vendor()

    def _save_uname(self):
        self.uname = _read_uname()
        self.last_uname_save = time.time()

    def _get_vendor(self):
        if self.m_distributor_id:
            vendor = self.m_distributor_id.lower()
            return vendor
        return None

    def _get_os_version(self):
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
            os_version = release.split(".") + [""]*3
            return os_version[:3]

        number_block_re = re.compile(r"\d+")
        if self.m_description:
            blocks = number_block_re.findall(self.m_description)
            blocks_length = len(blocks)
            if blocks_length > 0:
                block_trio = blocks + [""]*3
                return block_trio[:3]

        # Final fall-back - use kernel version
        kernel = self.kernel()
        if kernel:
            kernel_trio = kernel.strip().split(".") + [""]*3
            return kernel_trio[:3]

        return "", "", ""

    def detect_instance_info(self):
        """
        detect_instance_info
        :return:
        """
        return self.get_aws_info() or self.get_oracle_info() or self.get_azure_info() or self.get_google_info() or ""

    def get_aws_info(self):
        try:
            proxy_handler = urllib.request.ProxyHandler({})
            opener = urllib.request.build_opener(proxy_handler)
            request = urllib.request.Request('http://169.254.169.254/latest/api/token')
            request.add_header('X-aws-ec2-metadata-token-ttl-seconds', '21600')
            request.get_method = lambda: 'PUT'
            token = opener.open(request, timeout=1).read()

            request = urllib.request.Request('http://169.254.169.254/latest/dynamic/instance-identity/document')
            request.add_header('X-aws-ec2-metadata-token', token)

            aws_info_string = opener.open(request, timeout=1).read()
            aws_info = json.loads(aws_info_string)
            aws_info_json = {"platform": "aws",
                             "region": aws_info["region"],
                             "accountId": aws_info["accountId"],
                             "instanceId": aws_info["instanceId"]
                            }
            write_info_to_metadata_json(aws_info_json)

            return "".join((
                "<aws>",
                "<region>%s</region>" % aws_info["region"],
                "<accountId>%s</accountId>" % aws_info["accountId"],
                "<instanceId>%s</instanceId>" % aws_info["instanceId"],
                "</aws>"))
        except:  # pylint: disable=bare-except
            pass
        return None

    def get_google_info(self):
        try:
            proxy_handler = urllib.request.ProxyHandler({})
            opener = urllib.request.build_opener(proxy_handler)
            request = urllib.request.Request('http://metadata.google.internal/computeMetadata/v1/instance/id')
            request.add_header('Metadata-Flavor', "Google")
            id = opener.open(request, timeout=1).read().decode()

            request = urllib.request.Request('http://metadata.google.internal/computeMetadata/v1/instance/zone')
            request.add_header('Metadata-Flavor', "Google")
            zone = opener.open(request, timeout=1).read().decode()

            request = urllib.request.Request('http://metadata.google.internal/computeMetadata/v1/instance/hostname')
            request.add_header('Metadata-Flavor', "Google")
            hostname = opener.open(request, timeout=1).read().decode()
            google_info_json = {"platform": "google",
                             "hostname": hostname,
                             "id": id,
                             "zone": zone
                             }
            write_info_to_metadata_json(google_info_json)

            return "".join((
                "<google>",
                "<hostname>%s</hostname>" % hostname,
                "<id>%s</id>" % id,
                "<zone>%s</zone>" % zone,
                "</google>"))
        except:  # pylint: disable=bare-except
            pass
        return None

    def get_oracle_info(self):
        try:
            proxy_handler = urllib.request.ProxyHandler({})
            opener = urllib.request.build_opener(proxy_handler)

            request = urllib.request.Request('http://169.254.169.254/opc/v2/instance/')
            request.add_header('Authorization', "Bearer Oracle")

            oracle_info_string = opener.open(request, timeout=1).read()
            oracle_info = json.loads(oracle_info_string)
            oracle_info_json = {"platform": "oracle",
                                "region": oracle_info["region"],
                                "availabilityDomain": oracle_info["availabilityDomain"],
                                "compartmentId": oracle_info["compartmentId"],
                                "displayName": oracle_info["displayName"],
                                "hostname": oracle_info["hostname"],
                                "state": oracle_info["state"],
                                "instanceId": oracle_info["id"]
                                }
            write_info_to_metadata_json(oracle_info_json)
            return "".join((
                "<oracle>",
                "<region>%s</region>" % oracle_info["region"],
                "<availabilityDomain>%s</availabilityDomain>" % oracle_info["availabilityDomain"],
                "<compartmentId>%s</compartmentId>" % oracle_info["compartmentId"],
                "<displayName>%s</displayName>" % oracle_info["displayName"],
                "<hostname>%s</hostname>" % oracle_info["hostname"],
                "<state>%s</state>" % oracle_info["state"],
                "<instanceId>%s</instanceId>" % oracle_info["id"],
                "</oracle>"))
        except:  # pylint: disable=bare-except
            pass
        return None

    def get_azure_info(self):
        try:
            proxy_handler = urllib.request.ProxyHandler({})
            opener = urllib.request.build_opener(proxy_handler)

            request = urllib.request.Request('http://169.254.169.254/metadata/versions')
            request.add_header('Metadata', True)
            azure_info_string = opener.open(request, timeout=1).read()
            azure_version_info = json.loads(azure_info_string)
            latest_azure_api_version = azure_version_info["apiVersions"][-1]

            request = urllib.request.Request('http://169.254.169.254/metadata/instance?api-version=' + latest_azure_api_version)
            request.add_header('Metadata', True)

            azure_info_string = opener.open(request, timeout=1).read()
            azure_info = json.loads(azure_info_string)
            azure_info_json = {"platform": "azure",
                               "vmId": azure_info["compute"]["vmId"],
                               "vmName": azure_info["compute"]["name"],
                               "resourceGroupName": azure_info["compute"]["resourceGroupName"],
                               "subscriptionId": azure_info["compute"]["subscriptionId"]
                               }
            write_info_to_metadata_json(azure_info_json)

            return "".join((
                "<azure>",
                "<vmId>%s</vmId>" % azure_info["compute"]["vmId"],
                "<vmName>%s</vmName>" % azure_info["compute"]["name"],
                "<resourceGroupName>%s</resourceGroupName>" % azure_info["compute"]["resourceGroupName"],
                "<subscriptionId>%s</subscriptionId>" % azure_info["compute"]["subscriptionId"],
                "</azure>"))
        except:  # pylint: disable=bare-except
            pass
        return None

    def kernel(self):
        """
        Detect the kernel we are running on.
        """
        return self.uname[1]

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
        ten_mins = 600
        if abs(time.time() - self.last_uname_save) > ten_mins or self.uname[0] == "localhost":
            self._save_uname()
        return self.uname[0].split(".")[0]

    def vendor(self):
        """
        vendor
        :return:
        """
        if self.m_vendor:
            return self.m_vendor
        return "unknown"

    def os_version(self):
        """
        os_version
        :return:
        """
        return self._get_os_version()

    def os_name(self):
        """
        os_name
        :return:
        """
        if self.m_description:
            return self.m_description
        return "unknown"


def _read_uname():
    return [platform.node(), platform.release(), platform.machine()]


def _run_command(command, shell=False):
    try:
        proc = subprocess.Popen(command,
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


def _collect_os_release(os_info_length_limit=255):
    os_release_file_info = filesystem_utils.return_lines_from_file('/etc/os-release',
                                                                   ["NAME", "PRETTY_NAME", "VERSION_ID"])
    if os_release_file_info is None:
        return None, None, None

    for key, value in os_release_file_info.items():
        if len(value) != 1:
            os_release_file_info[key] = None  # Duplicate field found
            continue

        if not value[0].startswith(key + '='):
            os_release_file_info[key] = None  # Key/value is not in correct format
            continue

        split_line = value[0].split(key + '=')[1]
        if '=' in split_line:
            os_release_file_info[key] = None
            continue  # Format is key=<value1>=<value2>..., which is incorrect

        os_release_file_info[key] = split_line.strip().strip('\"')
        if len(os_release_file_info[key]) > os_info_length_limit:
            # Value is too large so just try to get the first word and see if it fits the limit
            os_release_file_info[key] = os_release_file_info[key].split(' ')[0]
        if len(os_release_file_info[key]) > os_info_length_limit:
            # Still too large so now truncate it
            os_release_file_info[key] = os_release_file_info[key][:os_info_length_limit]

    # If corresponding values were found then assign them, otherwise assign default value of None
    description = os_release_file_info.get("PRETTY_NAME", None)
    distributor_id = os_release_file_info.get("NAME", None)
    release = os_release_file_info.get("VERSION_ID", None)

    return description, distributor_id, release


def _find_case_insensitive_string_in_file(file_to_read, string_to_find):
    try:
        with open(file_to_read) as file:
            content = file.read()
            if string_to_find.lower() in content.lower():
                return True
    except EnvironmentError:
        pass
    return False


def write_info_to_metadata_json(metaDataAsJson):
    instance_metadata_json_filepath = path_manager.instance_metadata_config()
    tmp_path = os.path.join(path_manager.temp_dir(),"temp.json")
    try:
        body = json.dumps(metaDataAsJson)
        filesystem_utils.atomic_write(instance_metadata_json_filepath, tmp_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP, body)
        if os.getuid() == 0:
            os.chown(instance_metadata_json_filepath, get_ids.get_uid("sophos-spl-local"), get_ids.get_gid("sophos-spl-group"))
    except PermissionError as e:
        LOGGER.warning(f"Cannot update file {instance_metadata_json_filepath} with error : {e}")


if __name__ == '__main__':
    ts = TargetSystem()
    print("vendor = {}".format(ts.vendor()))
    print("os_name = {}".format(ts.os_name()))
