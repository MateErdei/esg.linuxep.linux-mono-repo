// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "Common/ApplicationConfiguration/IApplicationPathManager.h"
#include "CentralRegistration/MessageRelaySorter.h"
#include "Common/OSUtilitiesImpl/LocalIPImpl.h"

#include "CentralRegistration/Main.h"
#include "CentralRegistration/MessageRelayExtractor.h"
#include "cmcsrouter/Config.h"
#include "cmcsrouter/ConfigOptions.h"
#include <gtest/gtest.h>
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/MockSystemUtils.h"
#include "tests/Common/OSUtilitiesImpl/MockILocalIP.h"

class CentralRegistrationMainTests : public LogInitializedTests
{
protected:
    std::unique_ptr<::testing::StrictMock<MockFileSystem>> mockFileSystem_ =
        std::make_unique<::testing::StrictMock<MockFileSystem>>();
};

TEST_F(CentralRegistrationMainTests, extractor)
{
    std::string test{ "relay1:port1,1,id1;relay2:port2,2,id2;relay3:port3,3,id3" };
    std::vector<MCS::MessageRelay> result = extractMessageRelays(test);
    ASSERT_EQ(result.size(), 3);
    ASSERT_EQ(result[0].priority, 1);
    ASSERT_EQ(result[0].id, "id1");
    ASSERT_EQ(result[0].address, "relay1");
    ASSERT_EQ(result[0].port, "port1");
    ASSERT_EQ(result[1].priority, 2);
    ASSERT_EQ(result[1].id, "id2");
    ASSERT_EQ(result[1].address, "relay2");
    ASSERT_EQ(result[1].port, "port2");
    ASSERT_EQ(result[2].priority, 3);
    ASSERT_EQ(result[2].id, "id3");
    ASSERT_EQ(result[2].address, "relay3");
    ASSERT_EQ(result[2].port, "port3");
}

TEST_F(CentralRegistrationMainTests, MessageRelayExtractorFormat)
{
    ASSERT_TRUE(extractMessageRelays("relay1:port1,1").empty());
    ASSERT_TRUE(extractMessageRelays("relay1:port1,1,id1,extra").empty());
    ASSERT_TRUE(extractMessageRelays("relay1:port1,priority1,id1").empty());
    ASSERT_TRUE(extractMessageRelays("relay1,1,id1").empty());
    ASSERT_TRUE(extractMessageRelays("relay1:port1:port2,1,id1").empty());

    std::vector<MCS::MessageRelay> expected{ { 1, "id1", "relay1", "port1" } };
    ASSERT_EQ(extractMessageRelays(";;;relay1:port1,1,id1;;;"), expected);
}

TEST_F(CentralRegistrationMainTests, SortMessageRelays)
{
    Common::OSUtilitiesImpl::replaceLocalIP(std::make_unique<FakeILocalIP>(
        std::vector<std::string>{ "192.168.0.0", "10.0.0.0" }, std::vector<std::string>{ "fc00::" }));

    // clang-format off
    std::vector<MCS::MessageRelay> sortedMessageRelays{
        { 0, "id", "192.168.0.0", "port" },
        { 0, "id", "10.0.0.1", "port" },
        { 0, "id", "fc00::2", "port" },
        { 0, "id", "192.168.0.4", "port" },
        { 0, "id", "10.0.0.127", "port" },
        { 0, "id", "fc00::ff", "port" },
        { 0, "id", "192.168.1.0", "port" },
        { 0, "id", "10.0.2.0", "port" },
        { 0, "id", "fc00::4ff", "port" },
        { 1, "id", "192.168.0.0", "port" },
        { 1, "id", "10.0.0.1", "port" },
        { 1, "id", "fc00::2", "port" },
        { 1, "id", "192.168.0.4", "port" },
        { 1, "id", "10.0.0.127", "port" },
        { 1, "id", "fc00::ff", "port" },
        { 1, "id", "192.168.1.0", "port" },
        { 1, "id", "10.0.2.0", "port" },
        { 1, "id", "fc00::400", "port" },
    };
    // clang-format on

    // Permute the list
    std::vector<MCS::MessageRelay> mixedMessageRelays{
        sortedMessageRelays.at(17), sortedMessageRelays.at(16), sortedMessageRelays.at(15), sortedMessageRelays.at(14),
        sortedMessageRelays.at(13), sortedMessageRelays.at(12), sortedMessageRelays.at(7),  sortedMessageRelays.at(6),
        sortedMessageRelays.at(5),  sortedMessageRelays.at(4),  sortedMessageRelays.at(11), sortedMessageRelays.at(10),
        sortedMessageRelays.at(9),  sortedMessageRelays.at(8),  sortedMessageRelays.at(3),  sortedMessageRelays.at(2),
        sortedMessageRelays.at(1),  sortedMessageRelays.at(0),
    };

    ASSERT_EQ(sortMessageRelays(mixedMessageRelays), sortedMessageRelays);

    Common::OSUtilitiesImpl::restoreLocalIP();
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArguments)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

TEST_F(CentralRegistrationMainTests, space_in_group_name)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "", "--central-group=grou p1/grou p2", ""
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();

    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);

    EXPECT_EQ(configOptions.config[MCS::MCS_TOKEN], argValues[0]);
    EXPECT_EQ(configOptions.config[MCS::MCS_URL], argValues[1]);
    EXPECT_EQ(configOptions.config[MCS::CENTRAL_GROUP], "grou p1/grou p2");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsIncludingOptionals)
{
    std::vector<std::string> argValues{ "MCS_Token001",
                                        "https://MCS_URL",
                                        "--customer-token",
                                        "MCS_CustomerToken002",
                                        "--proxy-credentials",
                                        "proxyUsername:proxyPassword",
                                        "--message-relay",
                                        "relay1:port1,1,id1;relay2:port2,2,id2;relay3:port3,3,id3",
                                        "--version",
                                        "thininstallerVersion",
                                        "--central-group=group1/group2",
                                        "--products=antivirus,mdr" };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

    ASSERT_EQ(configOptions.messageRelays.size(), 3);
    ASSERT_EQ(configOptions.messageRelays[0].priority, 1);
    ASSERT_EQ(configOptions.messageRelays[0].id, "id1");
    ASSERT_EQ(configOptions.messageRelays[0].address, "relay1");
    ASSERT_EQ(configOptions.messageRelays[0].port, "port1");
    ASSERT_EQ(configOptions.messageRelays[1].priority, 2);
    ASSERT_EQ(configOptions.messageRelays[1].id, "id2");
    ASSERT_EQ(configOptions.messageRelays[1].address, "relay2");
    ASSERT_EQ(configOptions.messageRelays[1].port, "port2");
    ASSERT_EQ(configOptions.messageRelays[2].priority, 3);
    ASSERT_EQ(configOptions.messageRelays[2].id, "id3");
    ASSERT_EQ(configOptions.messageRelays[2].address, "relay3");
    ASSERT_EQ(configOptions.messageRelays[2].port, "port3");
}

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithProxyCreds)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return("user:password"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithMCS_CA_Override)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("MCS_CA")).WillOnce(Return("some_path"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return(""));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(true));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPSEnvProxySet)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return("https://secure_proxy:443"));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).Times(0);

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

TEST_F(CentralRegistrationMainTests, CanSuccessfullyProcessAndStoreCommandLineArgumentsWithHTTPEnvProxySet)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "https://MCS_URL", "--central-group=group1/group2", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("ALLOW_OVERRIDE_MCS_CA")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("PROXY_CREDENTIALS")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("https_proxy")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("HTTPS_PROXY")).WillOnce(Return(""));
    EXPECT_CALL(*mockSystemUtils, getEnvironmentVariable("http_proxy")).WillOnce(Return("http://non_secure_proxy:80"));

    EXPECT_CALL(
        *mockFileSystem_, exists(Common::ApplicationConfiguration::applicationPathManager().getMcsCaOverrideFlag()))
        .WillOnce(Return(false));
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
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

TEST_F(CentralRegistrationMainTests, FailsWhenNotEnoughArgsGiven)
{
    std::vector<std::string> argValues{ "CentralRegistration" };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(
        logMessage, ::testing::HasSubstr("Insufficient positional arguments given. Expecting 2: MCS Token, MCS URL"));
}

TEST_F(CentralRegistrationMainTests, FailsWhenArgTwoIsAnOptionalArg)
{
    std::vector<std::string> argValues{
        "--groups=group1/group2", "MCS_Token001", "https://MCS_URL", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(
        logMessage, ::testing::HasSubstr("Expecting MCS Token, found optional argument: --groups=group1/group2"));
}

TEST_F(CentralRegistrationMainTests, FailsWhenArgThreeIsAnOptionalArg)
{
    std::vector<std::string> argValues{
        "MCS_Token001", "--groups=group1/group2", "https://MCS_URL", "--products=antivirus,mdr"
    };

    auto mockSystemUtils = std::make_shared<StrictMock<MockSystemUtils>>();
    testing::internal::CaptureStderr();

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{ std::move(mockFileSystem_) };
    MCS::ConfigOptions configOptions = CentralRegistration::processCommandLineOptions(argValues, mockSystemUtils);
    std::string logMessage = internal::GetCapturedStderr();

    ASSERT_TRUE(configOptions.config.empty());
    ASSERT_THAT(logMessage, ::testing::HasSubstr("Expecting MCS URL, found optional argument: --groups=group1/group2"));
}