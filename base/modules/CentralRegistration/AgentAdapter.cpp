/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AgentAdapter.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/OSUtilitiesImpl/PlatformUtils.h>
#include <Common/OSUtilitiesImpl/LocalIPImpl.h>

#include <sstream>

namespace CentralRegistrationImpl
{
    AgentAdapter::AgentAdapter()
    : m_platformUtils(Common::OSUtilitiesImpl::PlatformUtils())
    {}

    AgentAdapter::AgentAdapter(Common::OSUtilitiesImpl::PlatformUtils platformUtils)
    : m_platformUtils(platformUtils)
    {}

    std::string AgentAdapter::getStatusXml() const
    {
        std::stringstream statusXml;
        statusXml << getStatusHeader() << getCommonStatusXml() << getOptionalStatusValues() << getPlatformStatus()
                  << getStatusFooter();
        return statusXml.str();
    }

    std::string AgentAdapter::getStatusHeader() const
    {
        std::stringstream header;
        header << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               << "<ns:computerStatus xmlns:ns=\"http://www.sophos.com/xml/mcs/computerstatus\">"
               << "<meta protocolVersion=\"1.0\" timestamp=\""
               << Common::UtilityImpl::TimeUtils::MessageTimeStamp(std::chrono::system_clock::now()) << " softwareVersion=\""
               << getSoftwareVersion() << "\" />";
        return header.str();
    }

    std::string AgentAdapter::getCommonStatusXml() const
    {
        Common::OSUtilitiesImpl::LocalIPImpl localIp;
        Common::OSUtilities::IPs ips = localIp.getLocalIPs();
        std::string ip4;
        std::string ip6;
        if (!ips.ip4collection.empty())
        {
            ip4 = ips.ip4collection[0].stringAddress();
        }
        if (!ips.ip6collection.empty())
        {
            ip6 = ips.ip6collection[0].stringAddress();
        }

        std::stringstream commonStatusXml;
        commonStatusXml << "<commonComputerStatus>"
                        << "<domainName>UNKNOWN</domainName>"
                        << "<computerName>" << m_platformUtils.getHostname() << "</computerName>"
                        << "<computerDescription></computerDescription>"
                        << "<isServer>true</isServer>"
                        << "<operatingSystem>" << m_platformUtils.getPlatform() << "</operatingSystem>"
                        << "<lastLoggedOnUser>"
                        << "root@" << m_platformUtils.getHostname() << "</lastLoggedOnUser>"
                        << "<ipv4>" << ip4 << "</ipv4>"
                        << "<ipv6>" << ip6 << "</ipv6>"
                        << "<fqdn>" << m_platformUtils.getHostname() << "</fqdn>"
                        << "<processorArchitecture>" << m_platformUtils.getArchitecture() << "</processorArchitecture>";
        return commonStatusXml.str();
    }

    std::string AgentAdapter::getOptionalStatusValues() const
    {
        // For Groups, Products, and IP addrs
        return "";
    }

    std::string AgentAdapter::getPlatformStatus() const
    {
        std::stringstream platformStatusXml;
        platformStatusXml << "<posixPlatformDetails>"
                          << "<productType>sspl</productType>"
                          << "<platform>" << m_platformUtils.getPlatform() << "</platform>"
                          << "<vendor>" << m_platformUtils.getVendor() << "</vendor>"
                          << "<isServer>1</isServer>"
                          << "<osName>" << m_platformUtils.getOsName() << "</osName>"
                          << "<kernelVersion>" << m_platformUtils.getKernelVersion() << "</kernelVersion>"
                          << "<osMajorVersion>" << m_platformUtils.getOsMajorVersion() << "</osMajorVersion>"
                          << "<osMinorVersion>" << m_platformUtils.getOsMinorVersion() << "</osMinorVersion>"
                          << "</posixPlatformDetails>";
        return platformStatusXml.str();
    }

    std::string AgentAdapter::getStatusFooter() const { return "</ns:computerStatus>"; }

    std::string AgentAdapter::getSoftwareVersion() const { return "9.9.9.9"; }

}
