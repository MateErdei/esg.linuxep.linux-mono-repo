// Copyright 2022-2023 Sophos Limited. All rights reserved.

// Product
#include "cmcsrouter/AgentAdapter.h"
// Test helpers
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockPlatformUtils.h"
#include "tests/Common/OSUtilitiesImpl/MockILocalIP.h"
// 3rd party
#include <gtest/gtest.h>
// C++ standard
#include <sstream>

class AgentAdapterTests : public LogInitializedTests
{
public:
    std::vector<std::string> ip4Addresses {
        "192.168.168.64",
        "125.184.182.125",
        "192.240.35.35",
        "172.6.251.34",
        "100.80.13.214"
    };

    std::vector<std::string> ip6Addresses {
        "f5eb:f7a4:323f:e898:f212:d0a6:3405:feec",
        "4d48:b9eb:62a5:4119:6e7f:89e9:a652:4941",
        "e36:e861:82a2:a473:2c24:7a07:7867:6a50",
        "8cc1:bec7:87c5:b668:62bf:ccaa:9f56:8f7f",
        "edbd:3bb8:bffb:4b6b:3536:2be7:3066:4276"
    };

    std::vector<std::string> macAddresses {
        "11-33-55-BB-DD-FF",
        "00-22-44-AA-CC-EE"
    };

    std::map<std::string, std::string> basicXmlContent{
        { "hostname", "testHostname"},
        { "fqdn", "testHostname.eng.sophos"},
        { "platform", "testLinux"},
        { "ip4Address", ip4Addresses[0]},
        { "ip4Address2", ip4Addresses[1]},
        { "ip4Address3", ip4Addresses[2]},
        { "ip4Address4", ip4Addresses[3]},
        { "ip4Address5", ip4Addresses[4]},
        { "ip6Address", ip6Addresses[0]},
        { "ip6Address2", ip6Addresses[1]},
        { "ip6Address3", ip6Addresses[2]},
        { "ip6Address4", ip6Addresses[3]},
        { "ip6Address5", ip6Addresses[4]},
        { "architecture", "testArchitecture"},
        { "vendor", "linux"},
        { "osName", "testOs"},
        { "kernelVersion", "1.2.3"},
        { "osMajorVersion", "1"},
        { "osMinorVersion", "2"},
        { "domainName", "UNKNOWN"}
    };

    std::map<std::string, std::string> optionalXmlContent {
        { "softwareVersion", "9.9.9.9"},
        { "product1", "av"},
        { "product2", "mtr"},
        { "deviceGroup", "fake/group"},
        { "macAddress1", macAddresses[0]},
        { "macAddress2", macAddresses[1]}
    };

    std::map<std::string, std::string> optionalXmlContentForParsing {
        { "softwareVersion", "9.9.9.9"},
        { "products", "av,mtr"},
        { "centralGroup", "fake/group"}
    };

    std::string fakeCloudXml{"<fakeCloud>cloudDetails</fakeCloud>"};

    std::string expectedXmlBodyContent(bool options)
    {
        std::stringstream expectedCommonComputerStatus;

        expectedCommonComputerStatus
            << "\" softwareVersion=\"";
        if(options)
        {
            expectedCommonComputerStatus << optionalXmlContent["versionNumber"];
        }
        expectedCommonComputerStatus
            << "\" />"
            << commonStatusXml(options)
            << cloudPlatformStatus()
            << platformStatus()
            << statusFooter();

        return expectedCommonComputerStatus.str();
    }

    std::string expectedXmlHeaderContent()
    {
        std::stringstream header;
        header << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
               << "<ns:computerStatus xmlns:ns=\"http://www.sophos.com/xml/mcs/computerstatus\">"
               << "<meta protocolVersion=\"1.0\" timestamp=\"";
        return header.str();
    }

    std::string commonStatusXml(bool options)
    {
        std::stringstream ipAddressesXml;
        ipAddressesXml << "<ipAddresses>"
                       << "<ipv4>" << basicXmlContent["ip4Address"] << "</ipv4>"
                       << "<ipv4>" << basicXmlContent["ip4Address2"] << "</ipv4>"
                       << "<ipv4>" << basicXmlContent["ip4Address3"] << "</ipv4>"
                       << "<ipv4>" << basicXmlContent["ip4Address4"] << "</ipv4>"
                       << "<ipv4>" << basicXmlContent["ip4Address5"] << "</ipv4>"
                       << "<ipv6>" << basicXmlContent["ip6Address"] << "</ipv6>"
                       << "<ipv6>" << basicXmlContent["ip6Address2"] << "</ipv6>"
                       << "<ipv6>" << basicXmlContent["ip6Address3"] << "</ipv6>"
                       << "<ipv6>" << basicXmlContent["ip6Address4"] << "</ipv6>"
                       << "<ipv6>" << basicXmlContent["ip6Address5"] << "</ipv6>"
                       << "</ipAddresses>";

        std::stringstream commonStatusXml;
        commonStatusXml << "<commonComputerStatus>"
                        << "<domainName>" << basicXmlContent["domainName"] << "</domainName>"
                        << "<computerName>" << basicXmlContent["hostname"] << "</computerName>"
                        << "<computerDescription></computerDescription>"
                        << "<isServer>true</isServer>"
                        << "<operatingSystem>" << basicXmlContent["platform"] << "</operatingSystem>"
                        << "<lastLoggedOnUser>"
                        << "root@" << basicXmlContent["hostname"] << "</lastLoggedOnUser>"
                        << "<ipv4>" << basicXmlContent["ip4Address"] << "</ipv4>"
                        << "<ipv6>" << basicXmlContent["ip6Address"] << "</ipv6>"
                        << "<fqdn>" << basicXmlContent["fqdn"] << "</fqdn>"
                        << "<processorArchitecture>" << basicXmlContent["architecture"] << "</processorArchitecture>";
        if (options)
        {
            commonStatusXml << optionalStatusValues(ipAddressesXml);
        }
        else
        {
            commonStatusXml << ipAddressesXml.str();
        }
        commonStatusXml << "</commonComputerStatus>";
        return commonStatusXml.str();
    }

    std::string optionalStatusValues(std::stringstream& ipAddressesXml)
    {
        std::stringstream optionalXml;
        optionalXml << "<productsToInstall>"
                    << "<product>" << optionalXmlContent["product1"] << "</product>"
                    << "<product>" << optionalXmlContent["product2"] << "</product>"
                    << "</productsToInstall>"
                    << "<deviceGroup>" << optionalXmlContent["deviceGroup"] << "</deviceGroup>"
                    << ipAddressesXml.str()
                    << "<macAddresses>"
                    << "<macAddress>" << optionalXmlContent["macAddress1"] << "</macAddress>"
                    << "<macAddress>" << optionalXmlContent["macAddress2"] << "</macAddress>"
                    << "</macAddresses>";
        return optionalXml.str();
    }

    [[nodiscard]] std::string cloudPlatformStatus() const
    {
        return fakeCloudXml;
    }

    std::string platformStatus()
    {
        std::stringstream platformStatusXml;
        platformStatusXml << "<posixPlatformDetails>"
                          << "<productType>sspl</productType>"
                          << "<platform>" << basicXmlContent["platform"] << "</platform>"
                          << "<vendor>" << basicXmlContent["vendor"] << "</vendor>"
                          << "<isServer>1</isServer>"
                          << "<osName>" << basicXmlContent["osName"] << "</osName>"
                          << "<kernelVersion>" << basicXmlContent["kernelVersion"] << "</kernelVersion>"
                          << "<osMajorVersion>" << basicXmlContent["osMajorVersion"] << "</osMajorVersion>"
                          << "<osMinorVersion>" << basicXmlContent["osMinorVersion"] << "</osMinorVersion>"
                          << "</posixPlatformDetails>";
        return platformStatusXml.str();
    }

    static std::string statusFooter() { return "</ns:computerStatus>"; }

};

TEST_F(AgentAdapterTests, testGetStatusXmlReturnsExpectedXmlWithNoOptions) // NOLINT
{
    auto mockPlatformUtil = std::make_shared<StrictMock<MockPlatformUtils>>();
    auto mockLocalIP = std::make_shared<StrictMock<MockILocalIP>>();
    std::vector<std::string> emptyStringVector;

    EXPECT_CALL(*mockLocalIP, getLocalInterfaces()).WillRepeatedly(Return(MockILocalIP::buildInterfacesHelper()));
    EXPECT_CALL(*mockPlatformUtil, sortInterfaces(_));
    EXPECT_CALL(*mockPlatformUtil, getHostname()).WillRepeatedly(Return(basicXmlContent["hostname"]));
    EXPECT_CALL(*mockPlatformUtil, getFQDN()).WillRepeatedly(Return(basicXmlContent["fqdn"]));
    EXPECT_CALL(*mockPlatformUtil, getPlatform()).WillRepeatedly(Return(basicXmlContent["platform"]));
    EXPECT_CALL(*mockPlatformUtil, getVendor()).WillRepeatedly(Return(basicXmlContent["vendor"]));
    EXPECT_CALL(*mockPlatformUtil, getArchitecture()).WillRepeatedly(Return(basicXmlContent["architecture"]));
    EXPECT_CALL(*mockPlatformUtil, getOsName()).WillRepeatedly(Return(basicXmlContent["osName"]));
    EXPECT_CALL(*mockPlatformUtil, getKernelVersion()).WillRepeatedly(Return(basicXmlContent["kernelVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMajorVersion()).WillRepeatedly(Return(basicXmlContent["osMajorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMinorVersion()).WillRepeatedly(Return(basicXmlContent["osMinorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getDomainname()).WillRepeatedly(Return(basicXmlContent["domainName"]));
    EXPECT_CALL(*mockPlatformUtil, getIp4Addresses(_)).WillRepeatedly(Return(ip4Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip4Addresses)).WillRepeatedly(Return(basicXmlContent["ip4Address"]));
    EXPECT_CALL(*mockPlatformUtil, getIp6Addresses(_)).WillRepeatedly(Return(ip6Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip6Addresses)).WillRepeatedly(Return(basicXmlContent["ip6Address"]));
    EXPECT_CALL(*mockPlatformUtil, getCloudPlatformMetadata(_)).WillRepeatedly(Return(fakeCloudXml));
    EXPECT_CALL(*mockPlatformUtil, getMacAddresses()).WillRepeatedly(Return(emptyStringVector));

    MCS::AgentAdapter adapter(mockPlatformUtil, mockLocalIP);

    std::map<std::string, std::string> emptyOptions;
    std::string actualStatusXml = adapter.getStatusXml(emptyOptions);

    std::string expectedXmlHeader = expectedXmlHeaderContent();
    std::string expectedXmlBody = expectedXmlBodyContent(false);
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXmlHeader));
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXmlBody));
}

TEST_F(AgentAdapterTests, testGetStatusXmlReturnsExpectedXmlWithMigrateFromSAV) // NOLINT
{
    auto mockPlatformUtil = std::make_shared<StrictMock<MockPlatformUtils>>();
    auto mockLocalIP = std::make_shared<StrictMock<MockILocalIP>>();
    std::vector<std::string> emptyStringVector;

    EXPECT_CALL(*mockLocalIP, getLocalInterfaces()).WillRepeatedly(Return(MockILocalIP::buildInterfacesHelper()));
    EXPECT_CALL(*mockPlatformUtil, sortInterfaces(_));
    EXPECT_CALL(*mockPlatformUtil, getHostname()).WillRepeatedly(Return(basicXmlContent["hostname"]));
    EXPECT_CALL(*mockPlatformUtil, getFQDN()).WillRepeatedly(Return(basicXmlContent["fqdn"]));
    EXPECT_CALL(*mockPlatformUtil, getPlatform()).WillRepeatedly(Return(basicXmlContent["platform"]));
    EXPECT_CALL(*mockPlatformUtil, getVendor()).WillRepeatedly(Return(basicXmlContent["vendor"]));
    EXPECT_CALL(*mockPlatformUtil, getArchitecture()).WillRepeatedly(Return(basicXmlContent["architecture"]));
    EXPECT_CALL(*mockPlatformUtil, getOsName()).WillRepeatedly(Return(basicXmlContent["osName"]));
    EXPECT_CALL(*mockPlatformUtil, getKernelVersion()).WillRepeatedly(Return(basicXmlContent["kernelVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMajorVersion()).WillRepeatedly(Return(basicXmlContent["osMajorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMinorVersion()).WillRepeatedly(Return(basicXmlContent["osMinorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getDomainname()).WillRepeatedly(Return(basicXmlContent["domainName"]));
    EXPECT_CALL(*mockPlatformUtil, getIp4Addresses(_)).WillRepeatedly(Return(ip4Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip4Addresses))
        .WillRepeatedly(Return(basicXmlContent["ip4Address"]));
    EXPECT_CALL(*mockPlatformUtil, getIp6Addresses(_)).WillRepeatedly(Return(ip6Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip6Addresses))
        .WillRepeatedly(Return(basicXmlContent["ip6Address"]));
    EXPECT_CALL(*mockPlatformUtil, getCloudPlatformMetadata(_)).WillRepeatedly(Return(fakeCloudXml));
    EXPECT_CALL(*mockPlatformUtil, getMacAddresses()).WillRepeatedly(Return(emptyStringVector));

    MCS::AgentAdapter adapter(mockPlatformUtil, mockLocalIP);

    std::map<std::string, std::string> emptyOptions;
    setenv("FORCE_UNINSTALL_SAV", "1", 1);
    std::string actualStatusXml = adapter.getStatusXml(emptyOptions);
    unsetenv("FORCE_UNINSTALL_SAV");

    std::string expectedXmlHeader = expectedXmlHeaderContent();
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXmlHeader));
    EXPECT_THAT(actualStatusXml, HasSubstr("<migratedFromSAV>1</migratedFromSAV></posixPlatformDetails>"));
}

TEST_F(AgentAdapterTests, testGetStatusXmlReturnsExpectedXmlWithOptions) // NOLINT
{
    auto mockPlatformUtil = std::make_shared<StrictMock<MockPlatformUtils>>();
    auto mockLocalIP = std::make_shared<StrictMock<MockILocalIP>>();

    EXPECT_CALL(*mockLocalIP, getLocalInterfaces()).WillOnce(Return(MockILocalIP::buildInterfacesHelper()));
    EXPECT_CALL(*mockPlatformUtil, sortInterfaces(_));
    EXPECT_CALL(*mockPlatformUtil, getHostname()).WillRepeatedly(Return(basicXmlContent["hostname"]));
    EXPECT_CALL(*mockPlatformUtil, getFQDN()).WillRepeatedly(Return(basicXmlContent["fqdn"]));
    EXPECT_CALL(*mockPlatformUtil, getPlatform()).WillRepeatedly(Return(basicXmlContent["platform"]));
    EXPECT_CALL(*mockPlatformUtil, getVendor()).WillRepeatedly(Return(basicXmlContent["vendor"]));
    EXPECT_CALL(*mockPlatformUtil, getArchitecture()).WillRepeatedly(Return(basicXmlContent["architecture"]));
    EXPECT_CALL(*mockPlatformUtil, getOsName()).WillRepeatedly(Return(basicXmlContent["osName"]));
    EXPECT_CALL(*mockPlatformUtil, getKernelVersion()).WillRepeatedly(Return(basicXmlContent["kernelVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMajorVersion()).WillRepeatedly(Return(basicXmlContent["osMajorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getOsMinorVersion()).WillRepeatedly(Return(basicXmlContent["osMinorVersion"]));
    EXPECT_CALL(*mockPlatformUtil, getDomainname()).WillRepeatedly(Return(basicXmlContent["domainName"]));
    EXPECT_CALL(*mockPlatformUtil, getIp4Addresses(_)).WillRepeatedly(Return(ip4Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip4Addresses)).WillRepeatedly(Return(basicXmlContent["ip4Address"]));
    EXPECT_CALL(*mockPlatformUtil, getIp6Addresses(_)).WillRepeatedly(Return(ip6Addresses));
    EXPECT_CALL(*mockPlatformUtil, getFirstIpAddress(ip6Addresses)).WillRepeatedly(Return(basicXmlContent["ip6Address"]));
    EXPECT_CALL(*mockPlatformUtil, getCloudPlatformMetadata(_)).WillRepeatedly(Return(fakeCloudXml));
    EXPECT_CALL(*mockPlatformUtil, getMacAddresses()).WillRepeatedly(Return(macAddresses));

    MCS::AgentAdapter adapter(mockPlatformUtil, mockLocalIP);

    std::string actualStatusXml = adapter.getStatusXml(optionalXmlContentForParsing);

    std::string expectedXmlHeader = expectedXmlHeaderContent();
    std::string expectedXmlBody = expectedXmlBodyContent(true);
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXmlHeader));
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXmlBody));
}
