/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AgentAdapter.h"

#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/OSUtilitiesImpl/PlatformUtils.h>
#include <Common/OSUtilitiesImpl/LocalIPImpl.h>

#include <sstream>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <Common/CurlWrapper/CurlWrapper.h>

namespace CentralRegistrationImpl
{
    AgentAdapter::AgentAdapter()
    : m_platformUtils(std::make_shared<Common::OSUtilitiesImpl::PlatformUtils>())
    {}

    AgentAdapter::AgentAdapter(std::shared_ptr<Common::OSUtilities::IPlatformUtils> platformUtils)
    : m_platformUtils(platformUtils)
    {}

    std::string AgentAdapter::getStatusXml() const
    {
        std::map<std::string, std::string> optionsConfig;
        std::stringstream statusXml;
        statusXml << getStatusHeader() << getCommonStatusXml() << getCloudPlatformsStatus(optionsConfig) << getPlatformStatus() << getStatusFooter();
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


        std::stringstream commonStatusXml;
        commonStatusXml << "<commonComputerStatus>"
                        << "<domainName>UNKNOWN</domainName>"
                        << "<computerName>" << m_platformUtils->getHostname() << "</computerName>"
                        << "<computerDescription></computerDescription>"
                        << "<isServer>true</isServer>"
                        << "<operatingSystem>" << m_platformUtils->getPlatform() << "</operatingSystem>"
                        << "<lastLoggedOnUser>"
                        << "root@" << m_platformUtils->getHostname() << "</lastLoggedOnUser>"
                        << "<ipv4>" << m_platformUtils->getIp4Address() << "</ipv4>"
                        << "<ipv6>" << m_platformUtils->getIp6Address() << "</ipv6>"
                        << "<fqdn>" << m_platformUtils->getHostname() << "</fqdn>"
                        << "<processorArchitecture>" << m_platformUtils->getArchitecture() << "</processorArchitecture>"
                        << getOptionalStatusValues()
                        << "</commonComputerStatus>";
        return commonStatusXml.str();
    }

    std::string AgentAdapter::getOptionalStatusValues() const
    {
        // For Groups, Products, and IP addrs
        return "";
    }

    std::string AgentAdapter::getCloudPlatformsStatus(std::map<std::string, std::string> optionsConfig) const
    {
        m_platformUtils->setProxyConfig(optionsConfig);
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client = std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        return m_platformUtils->getCloudPlatformMetadata(client);
    }

    std::string AgentAdapter::getPlatformStatus() const
    {
        std::stringstream platformStatusXml;
        platformStatusXml << "<posixPlatformDetails>"
                          << "<productType>sspl</productType>"
                          << "<platform>" << m_platformUtils->getPlatform() << "</platform>"
                          << "<vendor>" << m_platformUtils->getVendor() << "</vendor>"
                          << "<isServer>1</isServer>"
                          << "<osName>" << m_platformUtils->getOsName() << "</osName>"
                          << "<kernelVersion>" << m_platformUtils->getKernelVersion() << "</kernelVersion>"
                          << "<osMajorVersion>" << m_platformUtils->getOsMajorVersion() << "</osMajorVersion>"
                          << "<osMinorVersion>" << m_platformUtils->getOsMinorVersion() << "</osMinorVersion>"
                          << "</posixPlatformDetails>";
        return platformStatusXml.str();
    }

    std::string AgentAdapter::getStatusFooter() const { return "</ns:computerStatus>"; }

    std::string AgentAdapter::getSoftwareVersion() const { return "9.9.9.9"; }

}
