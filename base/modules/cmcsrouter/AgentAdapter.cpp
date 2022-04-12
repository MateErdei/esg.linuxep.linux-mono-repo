/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AgentAdapter.h"

#include <Common/CurlWrapper/CurlWrapper.h>
#include <Common/HttpRequestsImpl/HttpRequesterImpl.h>
#include <Common/OSUtilitiesImpl/PlatformUtils.h>
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

    std::string AgentAdapter::getStatusHeader(std::map<std::string, std::string>& configOptions) const
    {
        std::stringstream header;
        header << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               << "<ns:computerStatus xmlns:ns=\"http://www.sophos.com/xml/mcs/computerstatus\">"
               << "<meta protocolVersion=\"1.0\" timestamp=\""
               << Common::UtilityImpl::TimeUtils::MessageTimeStamp(std::chrono::system_clock::now())
               << "\" softwareVersion=\"" << getSoftwareVersion(configOptions) << "\" />";
        return header.str();
    }

    std::string AgentAdapter::getCommonStatusXml(std::map<std::string, std::string>& configOptions) const
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
                        << getOptionalStatusValues(configOptions)
                        << "</commonComputerStatus>";
        return commonStatusXml.str();
    }

    std::string AgentAdapter::getOptionalStatusValues(std::map<std::string, std::string>& configOptions) const
    {
        // For Groups, Products, and IP addresses
        std::stringstream optionals;

        std::string productsAsString = Common::UtilityImpl::StringUtils::replaceAll(configOptions["products"], " ", "");
        if(!productsAsString.empty())
        {
            std::stringstream productsToInstall;
            productsToInstall << "<productsToInstall>";
            if(productsAsString != "none")
            {
                std::vector<std::string> products = Common::UtilityImpl::StringUtils::splitString(productsAsString, ",");
                for(std::string product : products)
                {
                    if(!product.empty())
                    {
                        if(Common::XmlUtilities::Validation::isStringXmlValid(product))
                        {
                            productsToInstall << "<product>" << product << "</product>";
                        }
                    }
                }
            }
            productsToInstall << "</productsToInstall>";
            optionals << productsToInstall.str();
        }

        std::string deviceGroupAsString = configOptions["centralGroup"];
        if(!deviceGroupAsString.empty())
        {
            optionals << "<deviceGroup>" << deviceGroupAsString << "</deviceGroup>";
        }

        std::vector<std::string> ip4Addresses = m_platformUtils->getIp4Addresses();
        std::vector<std::string> ip6Addresses = m_platformUtils->getIp6Addresses();
        if(!ip4Addresses.empty() || !ip6Addresses.empty())
        {
            std::stringstream ipAddresses;
            ipAddresses << "<ipAddresses>";
            for(std::string ip4Address : ip4Addresses)
            {
                ipAddresses << "<ipv4>" << ip4Address << "</ipv4>";
            }
            for(std::string ip6Address : ip6Addresses)
            {
                ipAddresses << "<ipv6>" << ip6Address << "</ipv6>";
            }
            ipAddresses << "</ipAddresses>";
            optionals << ipAddresses.str();
        }

        std::vector<std::string> systemMacAddresses = m_platformUtils->getMacAddresses();
        if(!systemMacAddresses.empty())
        {
            std::stringstream macAddresses;
            macAddresses << "<macAddresses>";
            for(std::string macAddress : systemMacAddresses)
            {
                macAddresses << "<macAddress>" << macAddress << "</macAddress>";
            }
            macAddresses << "</macAddresses>";
            optionals << macAddresses.str();
        }
        return optionals.str();
    }

    std::string AgentAdapter::getCloudPlatformsStatus() const
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper = std::make_shared<Common::CurlWrapper::CurlWrapper>();
        auto client = Common::HttpRequestsImpl::HttpRequesterImpl(curlWrapper);
        return m_platformUtils->getCloudPlatformMetadata(&client);
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

    std::string AgentAdapter::getSoftwareVersion(std::map<std::string, std::string>& configOptions) const
    {   // function used for this in case we want to get the version number from somewhere else in the future
        return configOptions[MCS::VERSION_NUMBER];
    }

}
