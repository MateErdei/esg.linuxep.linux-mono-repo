
import os

import logging
logger = logging.getLogger(__name__)

import AdapterBase
import mcsrouter.utils.Timestamp

import mcsrouter.utils.TargetSystemManager
from mcsrouter import IPAddress
import mcsrouter.utils.PathManager as PathManager

def formatIPv6(i):
    assert ":" not in i
    assert len(i) == 32
    o = []
    countZero = 0
    while len(i) > 0:
        sub = i[:4]
        i = i[4:]

        while len(sub) > 0 and sub[0] == "0":
            sub=sub[1:]
        if sub == "":
            countZero += 1
            continue

        if countZero == 1:
            o.append("0")
        if countZero > 1:
            o.append("")

        countZero = 0
        o.append(sub)

    if countZero == 1:
        o.append("0")
    if countZero > 1:
        o.append(":")

    return ":".join(o)

class ComputerCommonStatus(object):
    """
    Class to represent the details that constitute the common computer status XML
    """
    def __init__(self, targetSystem):
        self.computerName = targetSystem.hostname()
        self.os = targetSystem.platform
        self.fqdn = IPAddress.getFQDN()
        self.user = "root@%s"% self.fqdn
        self.sls = targetSystem.checkIfUpdatableToSLS()
        self.arch = targetSystem.arch
        self.ipv4s = list(IPAddress.getNonLocalIPv4())
        self.ipv6s = list(IPAddress.getNonLocalIPv6())
        self.ipv6s = [ formatIPv6(i) for i in self.ipv6s ]

    def __eq__(self, other):
        return type(self) == type(other) and self.__dict__ == other.__dict__

    def __ne__(self, other):
        return not self == other

    def toStatusXml(self):
        ipv4 = self.ipv4s[0] if self.ipv4s else ""
        ipv6 = self.ipv6s[0] if self.ipv6s else ""

        result = [
            "<commonComputerStatus>",
            "<domainName>UNKNOWN</domainName>",
            "<computerName>%s</computerName>"%(self.computerName),
            "<computerDescription></computerDescription>",
            "<operatingSystem>%s</operatingSystem>"%self.os,
            "<lastLoggedOnUser>%s</lastLoggedOnUser>"%(self.user),
            "<ipv4>%s</ipv4>"%ipv4,
            "<ipv6>%s</ipv6>"%ipv6,
            "<fqdn>%s</fqdn>"%self.fqdn,
            "<processorArchitecture>%s</processorArchitecture>"%(self.arch)
            ]
        if self.ipv4s or self.ipv6s:
            result.append("<ipAddresses>")
            for ip in self.ipv4s:
                result.append("<ipv4>%s</ipv4>"%ip)
            for ip in self.ipv6s:
                result.append("<ipv6>%s</ipv6>"%ip)
            result.append("</ipAddresses>")

        result.append("</commonComputerStatus>")
        return "".join(result)


class AgentAdapter(AdapterBase.AdapterBase):
    def __init__(self, installdir=None):
        self.__m_last_status_time = None
        if installdir is not None:
            PathManager.INST = installdir

        self.__m_commonStatus = None

    def getAppId(self):
        return "AGENT"

    def getStatusTTL(self):
        return "PT10000S"

    def getTimestamp(self):
        return mcsrouter.utils.Timestamp.timestamp()

    def getStatusXml(self):
        return "".join((
            self.getStatusHeader(),
            self.getCommonStatusXml(),
            self.getPlatformStatus(),
            self.getStatusFooter()
            ))

    def getStatusHeader(self):
        return """<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
<meta protocolVersion="1.0" timestamp="%s"/>"""%(self.getTimestamp())

    def getStatusFooter(self):
        return """</ns:computerStatus>"""

    def __targetSystem(self):
        return mcsrouter.utils.TargetSystemManager.getTargetSystem(PathManager.installDir())

    def __createCommonStatus(self):
        ts = self.__targetSystem()
        assert ts is not None
        return ComputerCommonStatus(ts)

    def hasNewStatus(self):
        return self.__m_commonStatus != self.__createCommonStatus()

    def getCommonStatusXml(self):
        commonStatus = self.__createCommonStatus()

        if self.__m_commonStatus != commonStatus:
            self.__m_commonStatus = commonStatus

            logger.info("Reporting computerName=%s,fqdn=%s,IPv4=%s",
                self.__m_commonStatus.computerName,
                self.__m_commonStatus.fqdn,
                str(self.__m_commonStatus.ipv4s))

        return self.__m_commonStatus.toStatusXml()

    def getPlatformStatus(self):
        ts = self.__targetSystem()
        platform = ts.platform
        vendor = ts.vendor()
        kernel = ts.kernel
        osName = ts.osName
        return "".join((
            "<posixPlatformDetails>",
            "<platform>%s</platform>"%platform,
            "<vendor>%s</vendor>"%vendor,
            "<isServer>1</isServer>",
            "<osName>%s</osName>"%osName,
            "<kernelVersion>%s</kernelVersion>"%kernel,
            "</posixPlatformDetails>"))
