// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "Common/FileSystem/IFileSystemException.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "ManagementAgent/HealthStatusImpl/SerialiseFunctions.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"

#include <gtest/gtest.h>

namespace
{
    class SerialiseFunctionsTests : public testing::Test
    {
    private:
        Common::Logging::ConsoleLoggingSetup m_loggingSetup;
    };
}

TEST_F(SerialiseFunctionsTests, serialiseThreatDetectionPluginHealthStatusObj)
{
    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMap;

    pluginHealthStatusMap["AV"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMap["AV"].healthValue = 123;
    pluginHealthStatusMap["AV"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMap["AV"].displayName = "AV Plugin";

    pluginHealthStatusMap["RTD"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMap["RTD"].healthValue = 789;
    pluginHealthStatusMap["RTD"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMap["RTD"].displayName = "RTD Plugin";

    std::string jsonString = ManagementAgent::HealthStatusImpl::serialiseThreatHealth(pluginHealthStatusMap);
    ASSERT_EQ(
        jsonString,
        R"({"AV":{"displayName":"AV Plugin","healthType":4,"healthValue":123},"RTD":{"displayName":"RTD Plugin","healthType":4,"healthValue":789}})");
}

TEST_F(SerialiseFunctionsTests, deserialiseThreatDetectionPluginHealthStatusObj)
{
    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMapExpected;

    pluginHealthStatusMapExpected["AV"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMapExpected["AV"].healthValue = 1;
    pluginHealthStatusMapExpected["AV"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMapExpected["AV"].displayName = "AV Plugin";

    pluginHealthStatusMapExpected["RTD"] = ManagementAgent::PluginCommunication::PluginHealthStatus();
    pluginHealthStatusMapExpected["RTD"].healthValue = 2;
    pluginHealthStatusMapExpected["RTD"].healthType = ManagementAgent::PluginCommunication::HealthType::THREAT_DETECTION;
    pluginHealthStatusMapExpected["RTD"].displayName = "RTD Plugin";

    std::map<std::string, ManagementAgent::PluginCommunication::PluginHealthStatus> pluginHealthStatusMap =
        ManagementAgent::HealthStatusImpl::deserialiseThreatHealth(
            R"({"AV":{"displayName":"AV Plugin","healthType":4,"healthValue":1},"RTD":{"displayName":"RTD Plugin","healthType":4,"healthValue":2}})");

    ASSERT_EQ(pluginHealthStatusMap, pluginHealthStatusMapExpected);
}

TEST_F(SerialiseFunctionsTests, compareAndUpdateOverallHealthWhenHealthDoesNotChange)
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::string contents = "{\"health\":1,\"service\":1,\"threat\":1,\"threatService\":1}";
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(contents));
    std::pair<bool,std::string> info = ManagementAgent::HealthStatusImpl::compareAndUpdateOverallHealth(1,1,1,1);
    EXPECT_EQ(info.first,false);
}

TEST_F(SerialiseFunctionsTests, compareAndUpdateOverallHealthWhenHealthGoesBad)
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::string contents = "{\"health\":1,\"service\":1,\"threat\":1,\"threatService\":1}";
    std::string expected = "{\"health\":3,\"service\":3,\"threat\":1,\"threatService\":1}";
    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return(contents));
    std::pair<bool,std::string> info = ManagementAgent::HealthStatusImpl::compareAndUpdateOverallHealth(3,3,1,1);
    EXPECT_EQ(info.first,true);
    EXPECT_EQ(info.second,expected);
}

TEST_F(SerialiseFunctionsTests, compareAndUpdateOverallHealthReturnNewHealthIfParsingOldHealthFails)
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::string contents = "{\"health\":2,\"service\":2,\"threat\":1,\"threatService\":1}";

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(*filesystemMock, readFile(_)).WillOnce(Return("notJson"));
    std::pair<bool,std::string> info = ManagementAgent::HealthStatusImpl::compareAndUpdateOverallHealth(2,2,1,1);
    EXPECT_EQ(info.first,true);
    EXPECT_EQ(info.second,contents);

}

TEST_F(SerialiseFunctionsTests, compareAndUpdateOverallHealthReturnsNewHealthWhenNoOldHealth)
{
    auto filesystemMock = new NiceMock<MockFileSystem>();
    std::string contents = "{\"health\":2,\"service\":2,\"threat\":1,\"threatService\":1}";

    Tests::ScopedReplaceFileSystem scopedReplaceFileSystem{std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock)};
    EXPECT_CALL(*filesystemMock, isFile(_)).WillRepeatedly(Return(false));
    std::pair<bool,std::string> info = ManagementAgent::HealthStatusImpl::compareAndUpdateOverallHealth(2,2,1,1);
    EXPECT_EQ(info.first,true);
    EXPECT_EQ(info.second,contents);

}
