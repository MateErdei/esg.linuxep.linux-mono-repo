/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include <modules/pluginimpl/PluginUtils.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/FileSystem/IFileSystemException.h>
#include <modules/pluginimpl/ApplicationPaths.h>

#include <Common/Helpers/TempDir.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/MockFilePermissions.h>
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

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\nrunning_mode=0\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", true);
    ASSERT_EQ("disable_auditd=1\nrunning_mode=1\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
}

TEST_F(TestPluginUtils, setGivenFlagFromSettingsFileUpdatesFilefromXDRToEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=0\nrunning_mode=1\n");

    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    Plugin::PluginUtils::setGivenFlagFromSettingsFile("running_mode", false);
    ASSERT_EQ("disable_auditd=0\nrunning_mode=0\n",tempDir.fileContent("plugins/edr/etc/plugin.conf"));
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

class TestPluginUtilsWithMockFileSystem: public LogOffInitializedTests
{
public:
    TestPluginUtilsWithMockFileSystem()
    {
        mockFileSystem = new ::testing::NiceMock<MockFileSystem>();
        mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    }
    ~TestPluginUtilsWithMockFileSystem()
    {
        Tests::restoreFileSystem();
    }
    MockFileSystem * mockFileSystem;
    MockFilePermissions * mockFilePermissions;
};

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsDefaultsIfTargetDirectoryDoesNotExist)
{
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(false));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, Plugin::osqueryMTRConfigFilePath());
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsReturnsDefaultsIfTargetDirectoryEmpty)
{
    std::vector<std::string> files;
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, Plugin::osqueryMTRConfigFilePath());
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsReturnsDefaultsIfNeitherFileInDirectoryIsMTROrXDRPacks)
{
    std::vector<std::string> files;
    std::string filename = Plugin::osqueryCustomConfigFilePath();
    files.push_back(filename);
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, Plugin::osqueryMTRConfigFilePath());
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsOverridesXDRDefaultWhenDisabledXDRPackPresent)
{
    std::vector<std::string> files;
    std::string filename = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.DISABLED");
    files.push_back(filename);
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, Plugin::osqueryMTRConfigFilePath());
    EXPECT_EQ(targetFiles.second, filename);
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsOverridesMTRDefaultWhenDisabledMTRPackPresent)
{
    std::vector<std::string> files;
    std::string filename = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.mtr.DISABLED");
    files.push_back(filename);
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, filename);
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsOverridesMTRDefaultWhenMultipleMTRPacksPresent)
{
    std::vector<std::string> files;
    std::string filename1 = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.mtr.DISABLED");
    std::string filename2 = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.mtr.conf");
    files.push_back(filename1);
    files.push_back(filename2);
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, filename2);
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsOverridesDefaultsWhenDisabledPacksPresent)
{
    std::vector<std::string> files;
    std::string filename1 = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.mtr.DISABLED");
    std::string filename2 = Common::FileSystem::join(Plugin::osqueryConfigDirectoryPath(), "sophos-scheduled-query-pack.DISABLED");
    files.push_back(filename1);
    files.push_back(filename2);
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, filename1);
    EXPECT_EQ(targetFiles.second, filename2);
}

TEST_F(TestPluginUtilsWithMockFileSystem, getRunningQueryPackFilePathsReturnsDefaultsFileSystemExceptionIsThrown)
{
    Common::FileSystem::IFileSystemException e("test exception");
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Throw(e));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_EQ(targetFiles.first, Plugin::osqueryMTRConfigFilePath());
    EXPECT_EQ(targetFiles.second, Plugin::osqueryXDRConfigFilePath());
}

TEST_F(TestPluginUtilsWithMockFileSystem, nextQueryPacksShouldBeReloadedReturnsFalseWhenQueryPacksAreTheSame)
{
    std::vector<std::string> files;
    files.push_back(Plugin::osqueryMTRConfigFilePath());
    files.push_back(Plugin::osqueryXDRConfigFilePath());
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(_)).WillRepeatedly(Return("content"));
    EXPECT_FALSE(Plugin::PluginUtils::nextQueryPacksShouldBeReloaded());
}

TEST_F(TestPluginUtilsWithMockFileSystem, nextQueryPacksShouldBeReloadedReturnsTrueWhenMTRQueryPacksAreDifferent)
{
    std::vector<std::string> files;
    files.push_back(Plugin::osqueryMTRConfigFilePath());
    files.push_back(Plugin::osqueryXDRConfigFilePath());
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryMTRConfigFilePath())).WillOnce(Return("mtr_content1"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextMTRConfigStagingFilePath())).WillOnce(Return("mtr_content2"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryXDRConfigFilePath())).WillOnce(Return("xdr_content"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextXDRConfigStagingFilePath())).WillOnce(Return("xdr_content"));
    EXPECT_TRUE(Plugin::PluginUtils::nextQueryPacksShouldBeReloaded());
}

TEST_F(TestPluginUtilsWithMockFileSystem, nextQueryPacksShouldBeReloadedReturnsTrueWhenXDRQueryPacksAreDifferent)
{
    std::vector<std::string> files;
    files.push_back(Plugin::osqueryMTRConfigFilePath());
    files.push_back(Plugin::osqueryXDRConfigFilePath());
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryMTRConfigFilePath())).WillOnce(Return("mtr_content"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextMTRConfigStagingFilePath())).WillOnce(Return("mtr_content"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryXDRConfigFilePath())).WillOnce(Return("xdr_content1"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextXDRConfigStagingFilePath())).WillOnce(Return("xdr_content2"));
    EXPECT_TRUE(Plugin::PluginUtils::nextQueryPacksShouldBeReloaded());
}

TEST_F(TestPluginUtilsWithMockFileSystem, nextQueryPacksShouldBeReloadedReturnsTrueWhenAllQueryPacksAreDifferent)
{
    std::vector<std::string> files;
    files.push_back(Plugin::osqueryMTRConfigFilePath());
    files.push_back(Plugin::osqueryXDRConfigFilePath());
    EXPECT_CALL(*mockFileSystem, isDirectory(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, listFiles(Plugin::osqueryConfigDirectoryPath())).WillOnce(Return(files));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryMTRConfigFilePath())).WillOnce(Return("mtr_content1"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextMTRConfigStagingFilePath())).WillOnce(Return("mtr_content2"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryXDRConfigFilePath())).WillOnce(Return("xdr_content1"));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryNextXDRConfigStagingFilePath())).WillOnce(Return("xdr_content2"));
    EXPECT_TRUE(Plugin::PluginUtils::nextQueryPacksShouldBeReloaded());
}

TEST_F(TestPluginUtilsWithMockFileSystem, nextQueryPacksShouldBeReloadedReturnsDefaultOfTrueWhenFileSystemExceptionIsThrown)
{
    Common::FileSystem::IFileSystemException e("test exception");
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryMTRConfigFilePath())).WillOnce(Throw(e));
    std::pair<std::string,std::string> targetFiles = Plugin::PluginUtils::getRunningQueryPackFilePaths();
    EXPECT_TRUE(Plugin::PluginUtils::nextQueryPacksShouldBeReloaded());
}

TEST_F(TestPluginUtilsWithMockFileSystem, updatePluginConfWithFlagDoesNotUpdateFileOrVariableIfValueHasNotChanged)
{
    bool flagsHaveChanged = false;
    Tests::TempDir tempDir("/tmp");
    tempDir.createFile("plugins/edr/etc/plugin.conf", "flag_name=1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::edrConfigFilePath())).WillRepeatedly(Return(true));
    Plugin::PluginUtils::updatePluginConfWithFlag("flag_name", true, flagsHaveChanged);
    EXPECT_FALSE(flagsHaveChanged);
    std::string newContents = tempDir.fileContent("plugins/edr/etc/plugin.conf");
    EXPECT_EQ(newContents, "flag_name=1");
}

TEST_F(TestPluginUtilsWithMockFileSystem, updatePluginConfWithFlagUpdatesFileAndVariableIfValueHasBecomeTrue)
{
    bool flagsHaveChanged = false;
    std::vector<std::string> oldLines;
    oldLines.push_back("flag_name=0");
    Tests::TempDir tempDir("/tmp");
    tempDir.createFile("plugins/edr/etc/plugin.conf", "flag_name=0");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::edrConfigFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines(Plugin::edrConfigFilePath())).WillRepeatedly(Return(oldLines));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(Plugin::edrConfigFilePath(), "flag_name=1\n", Plugin::etcDir())).Times(1);
    Plugin::PluginUtils::updatePluginConfWithFlag("flag_name", true, flagsHaveChanged);
    EXPECT_TRUE(flagsHaveChanged);
}

TEST_F(TestPluginUtilsWithMockFileSystem, updatePluginConfWithFlagUpdatesFileAndVariableIfValueHasBecomeFalse)
{
    bool flagsHaveChanged = false;
    std::vector<std::string> oldLines;
    oldLines.push_back("flag_name=1");
    Tests::TempDir tempDir("/tmp");
    tempDir.createFile("plugins/edr/etc/plugin.conf", "flag_name=1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::edrConfigFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines(Plugin::edrConfigFilePath())).WillRepeatedly(Return(oldLines));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(Plugin::edrConfigFilePath(), "flag_name=0\n", Plugin::etcDir())).Times(1);
    Plugin::PluginUtils::updatePluginConfWithFlag("flag_name", false, flagsHaveChanged);
    EXPECT_TRUE(flagsHaveChanged);
}

TEST_F(TestPluginUtilsWithMockFileSystem, updatePluginConfWithFlagSetsValueInFileAndVariableIfFlagNotInFile)
{
    bool flagsHaveChanged = false;
    std::vector<std::string> oldLines;
    oldLines.push_back("other_name=1");
    Tests::TempDir tempDir("/tmp");
    tempDir.createFile("plugins/edr/etc/plugin.conf", "other_name=1");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
        Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::edrConfigFilePath())).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readLines(Plugin::edrConfigFilePath())).WillRepeatedly(Return(oldLines));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(Plugin::edrConfigFilePath(), "other_name=1\nflag_name=1\n", Plugin::etcDir())).Times(1);
    Plugin::PluginUtils::updatePluginConfWithFlag("flag_name", true, flagsHaveChanged);
    EXPECT_TRUE(flagsHaveChanged);
}
