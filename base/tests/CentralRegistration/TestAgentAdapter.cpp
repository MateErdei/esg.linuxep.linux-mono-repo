/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <CentralRegistration/AgentAdapter.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockPlatformUtils.h>

class AgentAdapterTests : public LogInitializedTests
{
public:

    std::string basicExpectedXmlContent(std::string options="")
    {
        std::string expectedCommonComputerStatus = R"(
            <commonComputerStatus>
            <domainName>UNKNOWN</domainName>
            <computerName>pair</computerName>
            <computerDescription></computerDescription>
            <isServer>true</isServer>
            <operatingSystem>linux</operatingSystem>
            <lastLoggedOnUser>root@pair</lastLoggedOnUser>
            <ipv4>192.168.168.64</ipv4>
            <ipv6>fe90::fce3:3ed2:a5ae:6ea9</ipv6>
            <fqdn>pair</fqdn>
            <processorArchitecture>x86_64</processorArchitecture>)";
        expectedCommonComputerStatus.append(options);
        expectedCommonComputerStatus.append("</commonComputerStatus>");

        return expectedCommonComputerStatus;
    }

    std::string basicExpectedPosixPlatformXmlContent()
    {
        std::string expectedPosixPlatformDetails = R"(
            <posixPlatformDetails>
            <productType>sspl</productType>
            <platform>linux</platform>
            <vendor>ubuntu</vendor>
            <isServer>1</isServer>
            <osName>osname</osName>
            <kernelVersion>kernel</kernelVersion>
            <osMajorVersion>12</osMajorVersion>
            <osMinorVersion>34</osMinorVersion>
            </posixPlatformDetails>
            </ns:computerStatus>)";
        return expectedPosixPlatformDetails;
    }

    std::vector<std::string> defaultReleaseFileContents()
    {
        // Test string used for lbs-release file matches content of ubuntu machine.
        std::vector<std::string> defaultContents = {
            "DISTRIB_ID=Ubuntu",
            "DISTRIB_RELEASE=18.04",
            "DISTRIB_CODENAME=bionic",
            "DISTRIB_DESCRIPTION=\"Ubuntu 18.04.6 LTS\""
        };
        return defaultContents;
    }
};
TEST_F(AgentAdapterTests, testGetStatusXmlReturnsExpectedXmlWithNoOptions) // NOLINT
{
    auto mockPlatformUtil = std::make_shared<StrictMock<MockPlatformUtils>>();
    //auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    //Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    std::string expectedXml = basicExpectedXmlContent().append(basicExpectedPosixPlatformXmlContent());
    expectedXml = Common::UtilityImpl::StringUtils::replaceAll(expectedXml, "\n", "");
    expectedXml = Common::UtilityImpl::StringUtils::replaceAll(expectedXml, "\t", "");
    expectedXml = Common::UtilityImpl::StringUtils::replaceAll(expectedXml, " ", "");

//    EXPECT_CALL(*mockFileSystem, readLines("/etc/lsb-release")).Times(2)
//        .WillRepeatedly(Return(defaultReleaseFileContents()));

    EXPECT_CALL(*mockPlatformUtil, getHostname()).WillRepeatedly(Return("pair"));
    EXPECT_CALL(*mockPlatformUtil, getPlatform()).WillRepeatedly(Return("linux"));
    EXPECT_CALL(*mockPlatformUtil, getArchitecture()).WillRepeatedly(Return("x86_64"));
    EXPECT_CALL(*mockPlatformUtil, getVendor()).WillRepeatedly(Return("ubuntu"));
    EXPECT_CALL(*mockPlatformUtil, getOsName()).WillRepeatedly(Return("osname"));
    EXPECT_CALL(*mockPlatformUtil, getKernelVersion()).WillRepeatedly(Return("kernel"));
    EXPECT_CALL(*mockPlatformUtil, getOsMajorVersion()).WillRepeatedly(Return("12"));
    EXPECT_CALL(*mockPlatformUtil, getOsMinorVersion()).WillRepeatedly(Return("34"));
    EXPECT_CALL(*mockPlatformUtil, getIp4Address()).WillRepeatedly(Return("192.168.168.64"));
    EXPECT_CALL(*mockPlatformUtil, getIp6Address()).WillRepeatedly(Return("fe90::fce3:3ed2:a5ae:6ea9"));

    CentralRegistrationImpl::AgentAdapter adapter(mockPlatformUtil);

    std::string actualStatusXml = adapter.getStatusXml();
    actualStatusXml = Common::UtilityImpl::StringUtils::replaceAll(actualStatusXml, " ", "");
    EXPECT_THAT(actualStatusXml, HasSubstr(expectedXml));

}
