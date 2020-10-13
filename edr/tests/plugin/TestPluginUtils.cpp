/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/PluginUtils.h>
#include <Common/FileSystem/IFileSystem.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>

class TestPluginUtils: public LogOffInitializedTests{};

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenIsXDR)
{
    std::string content = R"({"xdr.enabled" : "true"})";
    ASSERT_EQ(true,Plugin::PluginUtils::isRunningModeXDR(content));
}

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenIsNotXDR)
{
    std::string content = R"({"xdr.enabled" : "false"})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content));
}

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenInvalidJson)
{
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR("{}"));
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR("}"));
    std::string content = R"({xdr.enabled : "true"})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content));
    std::string content1 = R"({"xdr.enabled" : true})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content1));
}