/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AgentAdapter.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <Common/OSUtilitiesImpl/PlatformUtils.h>
#include <Common/OSUtilitiesImpl/SystemUtils.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <Common/XmlUtilities/Validation.h>

#include <iostream>

namespace MCS
{
    AgentAdapter::AgentAdapter()
    : m_platformUtils(std::make_shared<Common::OSUtilitiesImpl::PlatformUtils>())
    {}

    AgentAdapter::AgentAdapter(std::shared_ptr<Common::OSUtilities::IPlatformUtils> platformUtils)
    : m_platformUtils(platformUtils)
    {}

    std::string AgentAdapter::getStatusXml(std::map<std::string, std::string>& configOptions) const
    {
        std::stringstream statusXml;
        statusXml << getStatusHeader(configOptions)
                  << getCommonStatusXml(configOptions)
                  << getCloudPlatformsStatus()
                  << getPlatformStatus()
                  << getStatusFooter();
        return statusXml.str();
    }

    std::string AgentAdapter::getStatusHeader(const std::map<std::string, std::string>& configOptions) const
    {
        std::stringstream header;
        header << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               << "<ns:computerStatus xmlns:ns=\"http://www.sophos.com/xml/mcs/computerstatus\">"
               << "<meta protocolVersion=\"1.0\" timestamp=\""
               << Common::UtilityImpl::TimeUtils::MessageTimeStamp(std::chrono::system_clock::now())
               << "\" softwareVersion=\"";
        if (configOptions.find(MCS::VERSION_NUMBER) != configOptions.end())
        {
            header << configOptions.at(MCS::VERSION_NUMBER);
        }
        header << "\" />";
        return header.str();
    }

    std::string AgentAdapter::getCommonStatusXml(const std::map<std::string, std::string>& configOptions) const
    {
        std::stringstream commonStatusXml;
        commonStatusXml << "<commonComputerStatus>"
                        << "<domainName>" << m_platformUtils->getDomainname() << "</domainName>"
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
                        << getOptionalStatusValues(configOptions)
                        << "</commonComputerStatus>";
        return commonStatusXml.str();
    }

    std::string AgentAdapter::getOptionalStatusValues(const std::map<std::string, std::string>& configOptions) const
    {
        // For Groups, Products, and IP addresses
        std::stringstream optionals;

        if (configOptions.find(MCS::MCS_PRODUCTS) != configOptions.end())
        {
            std::string productsAsString = Common::UtilityImpl::StringUtils::replaceAll(configOptions.at(MCS::MCS_PRODUCTS), " ", "");
            optionals << "<productsToInstall>";
            if (productsAsString != "none")
            {
                std::vector<std::string> products = Common::UtilityImpl::StringUtils::splitString(productsAsString, ",");
                for (const std::string& product : products)
                {
                    if (!product.empty() && Common::XmlUtilities::Validation::stringWillNotBreakXmlParsing(product))
                    {
                        optionals << "<product>" << product << "</product>";
                    }
                }
            }
            optionals << "</productsToInstall>";
        }

        if (configOptions.find(MCS::CENTRAL_GROUP) != configOptions.end())
        {
            std::string deviceGroupAsString = configOptions.at(MCS::CENTRAL_GROUP);
            if (Common::XmlUtilities::Validation::stringWillNotBreakXmlParsing(deviceGroupAsString))
            {
                optionals << "<deviceGroup>" << deviceGroupAsString << "</deviceGroup>";
            }
        }

        std::vector<std::string> ip4Addresses = m_platformUtils->getIp4Addresses();
        std::vector<std::string> ip6Addresses = m_platformUtils->getIp6Addresses();
        if (!ip4Addresses.empty() || !ip6Addresses.empty())
        {
            optionals << "<ipAddresses>";
            for (const std::string& ip4Address : ip4Addresses)
            {
                optionals << "<ipv4>" << ip4Address << "</ipv4>";
            }
            for (const std::string& ip6Address : ip6Addresses)
            {
                optionals << "<ipv6>" << ip6Address << "</ipv6>";
            }
            optionals << "</ipAddresses>";
        }

        std::vector<std::string> systemMacAddresses = m_platformUtils->getMacAddresses();
        if (!systemMacAddresses.empty())
        {
            optionals << "<macAddresses>";
            for (const std::string& macAddress : systemMacAddresses)
            {
                optionals << "<macAddress>" << macAddress << "</macAddress>";
            }
            optionals << "</macAddresses>";
        }
        return optionals.str();
    }

    std::string AgentAdapter::getCloudPlatformsStatus() const
    {
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
                          << "<osMinorVersion>" << m_platformUtils->getOsMinorVersion() << "</osMinorVersion>";

        std::shared_ptr<OSUtilities::ISystemUtils> systemUtils = std::make_shared<OSUtilitiesImpl::SystemUtils>();
        if (systemUtils->getEnvironmentVariable("FORCE_UNINSTALL_SAV") == "1")
        {
            platformStatusXml << "<migratedFromSAV>1</migratedFromSAV>";
        }
        platformStatusXml << "</posixPlatformDetails>";
        return platformStatusXml.str();
    }

    std::string AgentAdapter::getStatusFooter() const { return "</ns:computerStatus>"; }

}
