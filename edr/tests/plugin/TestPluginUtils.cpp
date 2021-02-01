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

TEST_F(TestPluginUtils, testisFlagSetReturnsTruewhenFlagIsXDR)
{
    std::string content = R"({"xdr.enabled" : true})";
    ASSERT_EQ(true, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, content));
}

TEST_F(TestPluginUtils, testisFlagSetReturnsFalseWhenFlagIsNotXDR)
{
    std::string content = R"({"xdr.enabled" : false})";
    ASSERT_EQ(false, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, content));
}

TEST_F(TestPluginUtils, testisFlagSetReturnsFalseWhenInvalidJson)
{
    ASSERT_EQ(false, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, "{}"));
    ASSERT_EQ(false, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, "}"));
    std::string content = R"({xdr.enabled : true})";
    ASSERT_EQ(false, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, content));
    std::string content1 = R"({"xdr.enabled" : "true"})";
    ASSERT_EQ(false, Plugin::PluginUtils::isFlagSet(Plugin::PluginUtils::XDR_FLAG, content1));
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileWhenItIsEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "running_mode=0\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(false,Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"));
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileWhenItIsXDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\nrunning_mode=1\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(true,Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"));
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileThrowsWhenFileDoesNotExist)
{
    EXPECT_THROW(Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileThrowsWhenRunningModeNotSet)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileThrowsWhenSettingFileHasInvalidContent)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "blarg$$@~#\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"),std::runtime_error);
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileThrowsWhenSettingFileIsEmpty)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    EXPECT_THROW(Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"),std::runtime_error);
}

TEST_F(TestPluginUtils, setGivenFlagFromSettingsFileWhenFileIsEmpty)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", true);
    ASSERT_EQ("running_mode=1\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setGivenFlagFromSettingsFileWhenSettingFileDoesNotExist)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.makeDirs("plugins/edr/etc/");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", true);
    ASSERT_EQ("running_mode=1\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setGivenFlagFromSettingsFileUpdatesFilefromEDRToXDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\nrunning_mode=1\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", true);
    ASSERT_EQ("disable_auditd=1\nrunning_mode=1\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setGivenFlagFromSettingsFileUpdatesFilefromXDRToEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\nrunning_mode=1\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", false);
    ASSERT_EQ("disable_auditd=1\nrunning_mode=0\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, isInteger)
{
    ASSERT_EQ(true, Plugin::PluginUtils::isInteger("2"));
    ASSERT_EQ(true, Plugin::PluginUtils::isInteger("23456323"));
    ASSERT_EQ(false, Plugin::PluginUtils::isInteger("2 2"));
    ASSERT_EQ(false, Plugin::PluginUtils::isInteger("2.0"));
    ASSERT_EQ(true, Plugin::PluginUtils::isInteger("02"));
    ASSERT_EQ(true, Plugin::PluginUtils::isInteger(""));
}
