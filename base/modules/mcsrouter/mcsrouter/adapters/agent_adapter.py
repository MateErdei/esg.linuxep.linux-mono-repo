# Copyright 2019 Sophos Plc, Oxford, England.
"""
agent_adapter Module
"""

import logging
import os
import subprocess
import sys
import time

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.target_system_manager
import mcsrouter.utils.timestamp
from mcsrouter import ip_address

LOGGER = logging.getLogger(__name__)


def format_ipv6(ipv6):
    """
    format_ipv6
    """
    assert ":" not in ipv6
    assert len(ipv6) == 32
    result = []
    count_zero = 0
    while ipv6:
        sub = ipv6[:4]
        ipv6 = ipv6[4:]

        while sub and sub[0] == "0":
            sub = sub[1:]
        if sub == "":
            count_zero += 1
            continue

        if count_zero == 1:
            result.append("0")
        if count_zero > 1:
            result.append("")

        count_zero = 0
        result.append(sub)

    if count_zero == 1:
        result.append("0")
    if count_zero > 1:
        result.append(":")

    return ":".join(result)


class ComputerCommonStatus:
    """
    Class to represent the details that constitute the common computer status XML
    """
    # pylint: disable=too-many-instance-attributes, too-few-public-methods

    def __init__(self, target_system):
        """
        __init__
        """
        self.computer_name = target_system.hostname()
        self.operating_system = "linux"
        self.fqdn = ip_address.get_fqdn()
        self.user = "root@%s" % self.fqdn
        self.arch = target_system.architecture()
        self.ipv4s = list(ip_address.get_non_local_ipv4())
        self.ipv6s = list(ip_address.get_non_local_ipv6())
        self.ipv6s = [format_ipv6(i) for i in self.ipv6s]
        # The group that was passed into the thin installer with --group=<group> during installation
        self.install_time_central_group = get_installation_device_group()

        mac_addresses = []
        try:
            mac_addresses = self.get_mac_addresses()
        except PermissionError as e:
            LOGGER.error("Insufficient permission to run machineid. {}".format(e))
        self.mac_addresses = mac_addresses

    def __eq__(self, other):
        """
        __eq__
        """
        return isinstance(
            self, type(other)) and self.__dict__ == other.__dict__

    def __ne__(self, other):
        """
        __ne__
        """
        return not self == other

    def get_mac_addresses(self):
        path_to_machineid_executable = os.path.join(path_manager.install_dir(), "base", "bin", "machineid")

        command = [path_to_machineid_executable, "--dump-mac-addresses"]
        process = subprocess.Popen(command, stdout=subprocess.PIPE)

        stdout, stderr = process.communicate()
        mac_addresses = stdout.decode().split()
        return mac_addresses

    def to_status_xml(self, options=None):
        """
        to_status_xml
        """
        ipv4 = self.ipv4s[0] if self.ipv4s else ""
        ipv6 = self.ipv6s[0] if self.ipv6s else ""

        result = [
            "<commonComputerStatus>",
            "<domainName>UNKNOWN</domainName>",
            "<computerName>%s</computerName>" % (self.computer_name),
            "<computerDescription></computerDescription>",
            "<isServer>true</isServer>",
            "<operatingSystem>%s</operatingSystem>" % self.operating_system,
            "<lastLoggedOnUser>%s</lastLoggedOnUser>" % (self.user),
            "<ipv4>%s</ipv4>" % ipv4,
            "<ipv6>%s</ipv6>" % ipv6,
            "<fqdn>%s</fqdn>" % self.fqdn,
            "<processorArchitecture>%s</processorArchitecture>" % (self.arch)
        ]

        if self.install_time_central_group:
            result.append(f"<deviceGroup>{self.install_time_central_group}</deviceGroup>")

        if self.ipv4s or self.ipv6s:
            result.append("<ipAddresses>")
            for ip_addr in self.ipv4s:
                result.append("<ipv4>%s</ipv4>" % ip_addr)
            for ip_addr in self.ipv6s:
                result.append("<ipv6>%s</ipv6>" % ip_addr)
            result.append("</ipAddresses>")

        if options and options.selected_products:
            selected_products_elements = []
            invalid_xml = False
            selected_products_elements.append("<productsToInstall>")
            if options.selected_products == "none":
                LOGGER.info("Not requesting any products since argument is 'none'")
            else:
                for product in options.selected_products.split(","):
                    #factors out trailing comma
                    if product != "":
                        if is_string_xml_valid(product):
                            selected_products_elements.append("<product>%s</product>" % product)
                        else:
                            invalid_xml = True
                            LOGGER.warning(f"Product string: {product} is not safe to use as part xml request body")
            selected_products_elements.append("</productsToInstall>")
            if not invalid_xml:
                result.append("".join(selected_products_elements))

        if self.mac_addresses:
            result.append("<macAddresses>")
            for mac_address in self.mac_addresses:
                result.append("<macAddress>{}</macAddress>".format(mac_address))
            result.append("</macAddresses>")

        result.append("</commonComputerStatus>")
        return "".join(result)


def get_version():
    """
    get_version
    """
    try:
        version_location = os.path.join(path_manager.base_path(), "VERSION.ini")
        if os.path.isfile(version_location):
            with open(version_location) as version_file:
                for line in version_file.readlines():
                    line = line.strip()
                    if "PRODUCT_VERSION" in line:
                        version = line.split("=")[-1].strip()
                        return version
            LOGGER.error("PRODUCT_VERSION is not in VERSION.ini: Reporting softwareVersion=0 to Central")
        else:
            LOGGER.error("VERSION.ini file does not exist: Reporting softwareVersion=0 to Central")
    except PermissionError as e:
        LOGGER.error("Insufficient permissions to read VERSION.ini file: Reporting softwareVersion=0 to Central")
    return 0

def is_string_xml_valid(string: str):
    # Empty is invalid
    if string == '':
        return False
    # These characters are invalid and would break XML: <, &, >, ', "
    invalid_chars = ['<', '&', '>', "'", '"']
    for invalid_char in invalid_chars:
        if invalid_char in string:
            return False
    return True

def get_installation_device_group():
    """
    get_installation_device_group
    Extract the group to which the endpoint was installed, if present, from the install_options file
    """
    install_options_path = path_manager.install_options_file()
    try:
        if os.path.isfile(install_options_path):
            with open(install_options_path) as install_options_file:
                for line in install_options_file.readlines():
                    line = line.strip()
                    if "--group=" in line:
                        group = line.split("--group=")[-1]
                        if is_string_xml_valid(group):
                            LOGGER.debug(f"Central installation group found: {group}")
                            return group
                        else:
                            LOGGER.error("Malformed --group= option, device group will not be set.")
    except UnicodeDecodeError:
        LOGGER.error("Group cannot be decoded please run installer from a UTF-8 locale")
    except PermissionError:
        LOGGER.error(f"Insufficient permissions to read {install_options_path} file, device group will not be set.")
    except IndexError:
        LOGGER.error("Malformed --group= option, device group will not be set.")
    return None

class AgentAdapter(mcsrouter.adapters.adapter_base.AdapterBase):
    """
    AgentAdapter
    """
    # pylint: disable=no-self-use

    def __init__(self, install_dir=None):
        """
        __init__
        """
        self.__m_last_status_time = None
        if install_dir is not None:
            path_manager.INST = install_dir
        self.__m_common_status = None
        self.__m_created_time = None

    def get_app_id(self):
        """
        get_app_id
        """
        return "AGENT"

    def get_status_ttl(self):
        """
        get_status_ttl
        """
        return "PT10000S"

    def get_timestamp(self):
        """
        get_timestamp
        """
        return mcsrouter.utils.timestamp.timestamp()

    def get_status_xml(self, options=None):
        """
        get_status_xml
        """
        return "".join((
            self.get_status_header(),
            self.get_common_status_xml(options),
            self.get_aws_status(),  # Empty string if not aws
            self.get_platform_status(),
            self.get_policy_status(),
            self.get_status_footer()
        ))

    def get_status_header(self):
        """
        get_status_header
        """
        return """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
<meta protocolVersion="1.0" timestamp="{}" softwareVersion="{}" />""".format(self.get_timestamp(), get_version())

    def get_status_footer(self):
        """
        get_status_footer
        """
        return """</ns:computerStatus>"""

    def __target_system(self):
        """
        __target_system
        """
        return mcsrouter.utils.target_system_manager.get_target_system(
            path_manager.install_dir())

    def __get_common_status(self):
        """
        __create_common_status
        """
        #creates new status if has not been created or if status is over an hour old
        if self.__m_common_status:
            if not (time.time() - self.__m_created_time > 3600):
                return self.__m_common_status

        return self.__create_common_status()

    def __create_common_status(self):
        target_system = self.__target_system()
        assert target_system is not None
        self.__m_created_time = time.time()
        return ComputerCommonStatus(target_system)


    def has_new_status(self):
        """
        has_new_status
        """

        return self.__m_common_status != self.__get_common_status()

    def get_common_status_xml(self, options=None):
        """
        get_common_status_xml
        """
        self.__m_common_status = self.__get_common_status()
        return self.__m_common_status.to_status_xml(options)

    def get_platform_status(self):
        """
        get_platform_status
        """
        target_system = self.__target_system()
        platform = "linux"
        vendor = target_system.vendor()
        kernel = target_system.kernel()
        os_name = target_system.os_name()

        # should always be able to obtain first and second values from os_version
        os_version = target_system.os_version()
        version_length = len(os_version)

        if version_length == 0:
            major_version = ""
            minor_version = ""
            logging.warning("OS Version not found")
        elif version_length == 1:
            # this is expected on amazon Linux
            major_version = os_version[0]
            minor_version = ""
        else:
            major_version = os_version[0]
            minor_version = os_version[1]

        return "".join((
            "<posixPlatformDetails>",
            "<productType>sspl</productType>",
            "<platform>%s</platform>" % platform,
            "<vendor>%s</vendor>" % vendor,
            "<isServer>1</isServer>",
            "<osName>%s</osName>" % os_name,
            "<kernelVersion>%s</kernelVersion>" % kernel,
            "<osMajorVersion>%s</osMajorVersion>" % major_version,
            "<osMinorVersion>%s</osMinorVersion>" % minor_version,
            "</posixPlatformDetails>"))

    def get_aws_status(self):
        """
        get_aws_status
        """
        target_system = self.__target_system()
        aws_info = target_system.detect_instance_info()

        if aws_info is None:
            return ""

        region = aws_info["region"]
        account_id = aws_info["accountId"]
        instance_id = aws_info["instanceId"]

        return "".join((
            "<aws>",
            "<region>%s</region>" % region,
            "<accountId>%s</accountId>" % account_id,
            "<instanceId>%s</instanceId>" % instance_id,
            "</aws>"))

    def get_policy_status(self):
        """
        get_policy_status
        """

        entries = []
        latest_timestamp = mcsrouter.utils.timestamp.timestamp(0.0)
        policy_folder_path = os.path.join(path_manager.INST, "base/mcs/policy")
        policy_list = os.listdir(policy_folder_path)

        for filename in policy_list:
            filepath = os.path.join(policy_folder_path, filename)
            if os.path.isfile(filepath):
                epoch_timestamp = os.path.getmtime(filepath)
                windows_timestamp = mcsrouter.utils.timestamp.timestamp(epoch_timestamp)
                # Policies have three letter acronyms
                filename = filename[:3]

                if latest_timestamp > windows_timestamp:
                    latest_timestamp = windows_timestamp
                entries.append("<policy app=\"{}\" latest=\"{}\" />".format(filename, windows_timestamp))

        if not entries:
            return ""

        entries.insert(0, "<policy-status latest=\"{}\">".format(latest_timestamp))
        entries.append("</policy-status>")

        return "".join(entries)
