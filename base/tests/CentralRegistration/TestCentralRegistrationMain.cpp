/******************************************************************************************************

Copyright 2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ApplicationConfigurationImpl/ApplicationPathManager.h"

#include <CentralRegistration/Main.h>
#include <CentralRegistration/MessageReplayExtractor.h>
#include <cmcsrouter/Config.h>
#include <cmcsrouter/ConfigOptions.h>
#include <gtest/gtest.h>
#include <tests/Common/Helpers/FileSystemReplaceAndRestore.h>
#include <tests/Common/Helpers/LogInitializedTests.h>
#include <tests/Common/Helpers/MockFileSystem.h>
#include <tests/Common/Helpers/MockSystemUtils.h>

class CentralRegistrationMainTests : public LogInitializedTests
{
    virtual void TearDown()
    {
        Tests::restoreFileSystem();
    }
};

TEST_F(CentralRegistrationMainTests, extractor) //NOLINT
{
    std::string test{"relay1:port1,priority1,id1;relay2:port2,priority2,id2;relay3:port3,priority3,id3"};
    std::vector<MCS::MessageRelay> result = extractMessageRelays(test);
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].priority, "priority1");
    ASSERT_EQ(result[0].id, "id1");
    ASSERT_EQ(result[0].address, "relay1");
    ASSERT_EQ(result[0].port, "port1");
    ASSERT_EQ(result[1].priority, "priority2");
    ASSERT_EQ(result[1].id, "id2");
    ASSERT_EQ(result[1].address, "relay2");
    ASSERT_EQ(result[1].port, "port2");
    ASSERT_EQ(result[2].priority, "priority3");
    ASSERT_EQ(result[2].id, "id3");
    ASSERT_EQ(result[2].address, "relay3");
    ASSERT_EQ(result[2].port, "port3");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArguments) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_CERT], "");

}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsIncludingOptionals) // NOLINT
{
    std::vector<std::string> argValues{
        "MCS_Token001",
        "https://MCS_URL",
        "--customer-token", "MCS_CustomerToken002",
        "--proxy-credentials", "proxyUsername:proxyPassword",
        "--message-relay", "relay1:port1,priority1,id1;relay2:port2,priority2,id2;relay3:port3,priority3,id3",
        "--version", "thininstallerVersion",
        "--central-group=group1/group2",
        "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "proxyUsername");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "proxyPassword");
    ASSERT_EQ(configOptions.config[MCS::MCS_CERT], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_CUSTOMER_TOKEN], "MCS_CustomerToken002");
    ASSERT_EQ(configOptions.config[MCS::VERSION_NUMBER], "thininstallerVersion");

    ASSERT_EQ(configOptions.messageRelays[0].priority, "priority1");
    ASSERT_EQ(configOptions.messageRelays[0].id, "id1");
    ASSERT_EQ(configOptions.messageRelays[0].address, "relay1");
    ASSERT_EQ(configOptions.messageRelays[0].port, "port1");
    ASSERT_EQ(configOptions.messageRelays[1].priority, "priority2");
    ASSERT_EQ(configOptions.messageRelays[1].id, "id2");
    ASSERT_EQ(configOptions.messageRelays[1].address, "relay2");
    ASSERT_EQ(configOptions.messageRelays[1].port, "port2");
    ASSERT_EQ(configOptions.messageRelays[2].priority, "priority3");
    ASSERT_EQ(configOptions.messageRelays[2].id, "id3");
    ASSERT_EQ(configOptions.messageRelays[2].address, "relay3");
    ASSERT_EQ(configOptions.messageRelays[2].port, "port3");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithProxyCreds) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return("user:password"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "user");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "password");
    ASSERT_EQ(configOptions.config[MCS::MCS_CERT], "");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithMCS_CA_Override) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return("some_path"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag())).WillOnce(Return(true));
    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_CA_OVERRIDE], "some_path");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPSEnvProxySet) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>() ;

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return("https://secure_proxy:443"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).Times(0);

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "https://secure_proxy:443");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_CERT], "");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPEnvProxySet) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return("http://non_secure_proxy:80"));

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    ASSERT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    ASSERT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    ASSERT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "group1/group2");
    ASSERT_EQ(configOptions.config[MCS::MCS_PRODUCTS], "antivirus,mdr");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY], "http://non_secure_proxy:80");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_USERNAME], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_PROXY_PASSWORD], "");
    ASSERT_EQ(configOptions.config[MCS::MCS_CERT], "");
}

TEST_F(CentralRegistrationMainTests, FailsWhenNotEnoughArgsGiven) // NOLINT
{
    std::vector<std::string> argValues{
        "CentralRegistration"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Insufficient positional arguments given. Expecting 2: MCS Token, MCS URL"));
}

TEST_F(CentralRegistrationMainTests, FailsWhenArgTwoIsAnOptionalArg) // NOLINT
{
    std::vector<std::string> argValues{"--groups=group1/group2", "MCS_Token001", "https://MCS_URL", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Expecting MCS Token, found optional argument: --groups=group1/group2"));
}

TEST_F(CentralRegistrationMainTests, FailsWhenArgThreeIsAnOptionalArg) // NOLINT
{
    std::vector<std::string> argValues{"MCS_Token001", "--groups=group1/group2", "https://MCS_URL", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Expecting MCS URL, found optional argument: --groups=group1/group2"));
}