"""
agent_adapter Module
"""

import logging

from mcsrouter import ip_address

import mcsrouter.adapters.adapter_base
import mcsrouter.utils.timestamp
import mcsrouter.utils.target_system_manager
import mcsrouter.utils.path_manager as path_manager

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


class ComputerCommonStatus(object):
    """
    Class to represent the details that constitute the common computer status XML
    """
    # pylint: disable=too-many-instance-attributes, too-few-public-methods

    def __init__(self, target_system):
        """
        __init__
        """
        self.computer_name = target_system.hostname()
        self.operating_system = target_system.platform
        self.fqdn = ip_address.get_fqdn()
        self.user = "root@%s" % self.fqdn
        self.sls = target_system.check_if_updatable_to_sls()
        self.arch = target_system.arch
        self.ipv4s = list(ip_address.get_non_local_ipv4())
        self.ipv6s = list(ip_address.get_non_local_ipv6())
        self.ipv6s = [format_ipv6(i) for i in self.ipv6s]

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

    def to_status_xml(self):
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
            "<operatingSystem>%s</operatingSystem>" % self.operating_system,
            "<lastLoggedOnUser>%s</lastLoggedOnUser>" % (self.user),
            "<ipv4>%s</ipv4>" % ipv4,
            "<ipv6>%s</ipv6>" % ipv6,
            "<fqdn>%s</fqdn>" % self.fqdn,
            "<processorArchitecture>%s</processorArchitecture>" % (self.arch)
        ]
        if self.ipv4s or self.ipv6s:
            result.append("<ipAddresses>")
            for ip_addr in self.ipv4s:
                result.append("<ipv4>%s</ipv4>" % ip_addr)
            for ip_addr in self.ipv6s:
                result.append("<ipv6>%s</ipv6>" % ip_addr)
            result.append("</ipAddresses>")

        result.append("</commonComputerStatus>")
        return "".join(result)


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

    def get_status_xml(self):
        """
        get_status_xml
        """
        return "".join((
            self.get_status_header(),
            self.get_common_status_xml(),
            self.get_platform_status(),
            self.get_status_footer()
        ))

    def get_status_header(self):
        """
        get_status_header
        """
        return """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
<meta protocolVersion="1.0" timestamp="%s"/>""" % (self.get_timestamp())

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

    def __create_common_status(self):
        """
        __create_common_status
        """
        target_system = self.__target_system()
        assert target_system is not None
        return ComputerCommonStatus(target_system)

    def has_new_status(self):
        """
        has_new_status
        """
        return self.__m_common_status != self.__create_common_status()

    def get_common_status_xml(self):
        """
        get_common_status_xml
        """
        common_status = self.__create_common_status()

        if self.__m_common_status != common_status:
            self.__m_common_status = common_status

            LOGGER.info("Reporting computerName=%s,fqdn=%s,IPv4=%s",
                        self.__m_common_status.computer_name,
                        self.__m_common_status.fqdn,
                        str(self.__m_common_status.ipv4s))

        return self.__m_common_status.to_status_xml()

    def get_platform_status(self):
        """
        get_platform_status
        """
        target_system = self.__target_system()
        platform = target_system.platform
        vendor = target_system.vendor()
        kernel = target_system.kernel
        os_name = target_system.os_name
        return "".join((
            "<posixPlatformDetails>",
            "<productType>sspl</productType>",
            "<platform>%s</platform>" % platform,
            "<vendor>%s</vendor>" % vendor,
            "<isServer>1</isServer>",
            "<osName>%s</osName>" % os_name,
            "<kernelVersion>%s</kernelVersion>" % kernel,
            "</posixPlatformDetails>"))
