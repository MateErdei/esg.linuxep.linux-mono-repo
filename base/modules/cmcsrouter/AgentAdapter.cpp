// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "AgentAdapter.h"

#include "Common/CurlWrapper/CurlWrapper.h"
#include "Common/HttpRequestsImpl/HttpRequesterImpl.h"
#include "Common/OSUtilities/IIPUtils.h"
#include "Common/OSUtilitiesImpl/LocalIPImpl.h"
#include "Common/OSUtilitiesImpl/PlatformUtils.h"
#include "Common/OSUtilitiesImpl/SystemUtils.h"
#include "Common/UtilityImpl/StringUtils.h"
#include "Common/UtilityImpl/TimeUtils.h"
#include "Common/XmlUtilities/Validation.h"

#include <iostream>

#define MAX_OS_NAME_LENGTH 255

namespace MCS
{
    AgentAdapter::AgentAdapter() :
        m_platformUtils(std::make_shared<Common::OSUtilitiesImpl::PlatformUtils>()),
        m_localIp(std::make_shared<Common::OSUtilitiesImpl::LocalIPImpl>())
    {
    }

    AgentAdapter::AgentAdapter(
        std::shared_ptr<Common::OSUtilities::IPlatformUtils> platformUtils,
        std::shared_ptr<Common::OSUtilities::ILocalIP> localIp) :
        m_platformUtils(std::move(platformUtils)), m_localIp(std::move(localIp))
    {
    }

    std::string AgentAdapter::getStatusXml(
        std::map<std::string, std::string>& configOptions,
        const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const
    {
        std::stringstream statusXml;
        statusXml << getStatusHeader(configOptions) << getCommonStatusXml(configOptions)
                  << getCloudPlatformsStatus(client) << getPlatformStatus() << getStatusFooter();
        return statusXml.str();
    }

    std::string AgentAdapter::getStatusXml(std::map<std::string, std::string>& configOptions) const
    {
        std::shared_ptr<Common::CurlWrapper::ICurlWrapper> curlWrapper =
            std::make_shared<Common::CurlWrapper::CurlWrapper>();
        std::shared_ptr<Common::HttpRequests::IHttpRequester> client =
            std::make_shared<Common::HttpRequestsImpl::HttpRequesterImpl>(curlWrapper);
        return getStatusXml(configOptions, client);
    }

    std::string AgentAdapter::getStatusHeader(const std::map<std::string, std::string>& configOptions) const
    {
        std::stringstream header;
        header << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
               << "<ns:computerStatus xmlns:ns=\"http://www.sophos.com/xml/mcs/computerstatus\">"
               << R"(<meta protocolVersion="1.0" timestamp=")"
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

        std::vector<Common::OSUtilities::Interface> interfaces = m_localIp->getLocalInterfaces();
        m_platformUtils->sortInterfaces(interfaces);
        std::vector<std::string> ip4Addresses = m_platformUtils->getIp4Addresses(interfaces);
        std::vector<std::string> ip6Addresses = m_platformUtils->getIp6Addresses(interfaces);

        commonStatusXml << "<commonComputerStatus>"
                        << "<domainName>" << m_platformUtils->getDomainname() << "</domainName>"
                        << "<computerName>" << m_platformUtils->getHostname() << "</computerName>"
                        << "<computerDescription></computerDescription>"
                        << "<isServer>true</isServer>"
                        << "<operatingSystem>" << m_platformUtils->getPlatform() << "</operatingSystem>"
                        << "<lastLoggedOnUser>"
                        << "root@" << m_platformUtils->getHostname() << "</lastLoggedOnUser>"
                        << "<ipv4>" << m_platformUtils->getFirstIpAddress(ip4Addresses) << "</ipv4>"
                        << "<ipv6>" << m_platformUtils->getFirstIpAddress(ip6Addresses) << "</ipv6>"
                        << "<fqdn>" << m_platformUtils->getFQDN() << "</fqdn>"
                        << "<processorArchitecture>" << m_platformUtils->getArchitecture() << "</processorArchitecture>"
                        << getOptionalStatusValues(configOptions, ip4Addresses, ip6Addresses)
                        << "</commonComputerStatus>";
        return commonStatusXml.str();
    }

    std::string AgentAdapter::getOptionalStatusValues(
        const std::map<std::string, std::string>& configOptions,
        const std::vector<std::string>& ip4Addresses,
        const std::vector<std::string>& ip6Addresses) const
    {
        // For Groups, Products, and IP addresses
        std::stringstream optionals;

        if (configOptions.find(MCS::MCS_PRODUCTS) != configOptions.end())
        {
            std::string productsAsString =
                Common::UtilityImpl::StringUtils::replaceAll(configOptions.at(MCS::MCS_PRODUCTS), " ", "");
            optionals << "<productsToInstall>";
            if (productsAsString != "none")
            {
                std::vector<std::string> products =
                    Common::UtilityImpl::StringUtils::splitString(productsAsString, ",");
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

    std::string AgentAdapter::getCloudPlatformsStatus(
        const std::shared_ptr<Common::HttpRequests::IHttpRequester>& client) const
    {
        return m_platformUtils->getCloudPlatformMetadata(client);
    }

    std::string AgentAdapter::getPlatformStatus() const
    {
        std::stringstream platformStatusXml;
        std::string osName = m_platformUtils->getOsName();
        if (osName.size() > MAX_OS_NAME_LENGTH)
        {
            osName = osName.substr(0, MAX_OS_NAME_LENGTH);
        }
        platformStatusXml << "<posixPlatformDetails>"
                          << "<productType>sspl</productType>"
                          << "<platform>" << m_platformUtils->getPlatform() << "</platform>"
                          << "<vendor>" << m_platformUtils->getVendor() << "</vendor>"
                          << "<isServer>1</isServer>"
                          << "<osName>" << osName << "</osName>"
                          << "<kernelVersion>" << m_platformUtils->getKernelVersion() << "</kernelVersion>"
                          << "<osMajorVersion>" << m_platformUtils->getOsMajorVersion() << "</osMajorVersion>"
                          << "<osMinorVersion>" << m_platformUtils->getOsMinorVersion() << "</osMinorVersion>";

        std::shared_ptr<OSUtilities::ISystemUtils> systemUtils = std::make_shared<OSUtilities::SystemUtilsImpl>();
        if (systemUtils->getEnvironmentVariable("FORCE_UNINSTALL_SAV") == "1")
        {
            platformStatusXml << "<migratedFromSAV>1</migratedFromSAV>";
        }
        platformStatusXml << "</posixPlatformDetails>";
        return platformStatusXml.str();
    }

    std::string AgentAdapter::getStatusFooter() const
    {
        return "</ns:computerStatus>";
    }
} // namespace MCS
