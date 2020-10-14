/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/PluginUtils.h>
#include <Common/FileSystem/IFileSystem.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>
#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>

class TestPluginUtils: public LogOffInitializedTests{};

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenIsXDR)
{
    std::string content = R"({"xdr.enabled" : true})";
    ASSERT_EQ(true,Plugin::PluginUtils::isRunningModeXDR(content));
}

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenIsNotXDR)
{
    std::string content = R"({"xdr.enabled" : false})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content));
}

TEST_F(TestPluginUtils, testgetRunningModeFromFlagContentwhenInvalidJson)
{
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR("{}"));
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR("}"));
    std::string content = R"({xdr.enabled : true})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content));
    std::string content1 = R"({"xdr.enabled" : "true"})";
    ASSERT_EQ(false,Plugin::PluginUtils::isRunningModeXDR(content1));
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileWhenItIsEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "running_mode=1\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(false,Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile());
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileWhenItIsXDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\nrunning_mode=0\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(true,Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile());
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileThrowsWhenFileDoesNotExist)
{
    EXPECT_THROW(Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile(),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileThrowsWhenRunningModeNotSet)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile(),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileThrowsWhenSettingFileHasInvalidContent)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "blarg$$@~#\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile(),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveRunningModeFlagFromSettingsFileThrowsWhenSettingFileIsEmpty)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveRunningModeFlagFromSettingsFile(),std::runtime_error);
}

TEST_F(TestPluginUtils, setRunningModeFlagFromSettingsFileWhenFileIsEmpty)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setRunningModeFlagFromSettingsFile(true);
    ASSERT_EQ("running_mode=0\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setRunningModeFlagFromSettingsFileWhenSettingFileDoesNotExist)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.makeDirs("plugins/edr/etc/");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setRunningModeFlagFromSettingsFile(true);
    ASSERT_EQ("running_mode=0\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setRunningModeFlagFromSettingsFileUpdatesFilefromEDRToXDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\nrunning_mode=1\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setRunningModeFlagFromSettingsFile(true);
    ASSERT_EQ("disable_auditd=0\nrunning_mode=0\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setRunningModeFlagFromSettingsFileUpdatesFilefromXDRToEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\nrunning_mode=0\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setRunningModeFlagFromSettingsFile(false);
    ASSERT_EQ("disable_auditd=0\nrunning_mode=1\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}
