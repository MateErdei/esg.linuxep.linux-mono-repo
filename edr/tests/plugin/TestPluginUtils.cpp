// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "EdrCommon/ApplicationPaths.h"
#include "pluginimpl/PluginUtils.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/MockFilePermissions.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFilePermissions.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#endif

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/FileSystem/IFileSystemException.h"

#include <gtest/gtest.h>

class TestPluginUtils: public LogOffInitializedTests{};

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileWhenItIsEDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "running_mode=0\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(false, Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"));
}

TEST_F(TestPluginUtils, retrieveGivenFlagFromSettingsFileWhenItIsXDR)
{
    Tests::TempDir tempDir("/tmp");

    tempDir.createFile("plugins/edr/etc/plugin.conf", "disable_auditd=1\nrunning_mode=1\n");
    Common::ApplicationConfiguration::applicationConfiguration().setData(
            Common::ApplicationConfiguration::SOPHOS_INSTALL, tempDir.dirPath());

    ASSERT_EQ(true, Plugin::PluginUtils::retrieveGivenFlagFromSettingsFile("running_mode"));
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

    tempDir.createFile("plugins/edr/etc/plugin.conf", "running_mode=0\nblarg$$@~#\n");
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

class TestPluginUtilsWithStrictMockFileSystem: public LogOffInitializedTests
{
public:
    TestPluginUtilsWithStrictMockFileSystem()
    {
        mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
        mockFilePermissions = new ::testing::StrictMock<MockFilePermissions>();
        Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
        Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    }
    ~TestPluginUtilsWithStrictMockFileSystem()
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
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::edrConfigFilePath())).WillOnce(Return("flag_name=1"));
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
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::edrConfigFilePath())).WillOnce(Return("flag_name=0"));
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
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::edrConfigFilePath())).WillOnce(Return("flag_name=1"));
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
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::edrConfigFilePath())).WillOnce(Return("other_name=1"));
    EXPECT_CALL(*mockFileSystem, readLines(Plugin::edrConfigFilePath())).WillRepeatedly(Return(oldLines));
    EXPECT_CALL(*mockFileSystem, writeFileAtomically(Plugin::edrConfigFilePath(), "other_name=1\nflag_name=1\n", Plugin::etcDir())).Times(1);
    Plugin::PluginUtils::updatePluginConfWithFlag("flag_name", true, flagsHaveChanged);
    EXPECT_TRUE(flagsHaveChanged);
}

TEST_F(TestPluginUtilsWithMockFileSystem, enableAnddisableQueryPackRenamesQueryPack) // NOLINT
{
    std::string queryPackPath = "querypackpath";
    std::string queryPackPathDisabled = "querypackpath.DISABLED";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(_)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPathDisabled, queryPackPath));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPath, queryPackPathDisabled));
    Plugin::PluginUtils::enableQueryPack(queryPackPath);
    Plugin::PluginUtils::disableQueryPack(queryPackPath);
}

TEST_F(TestPluginUtilsWithMockFileSystem, enableQueryPackReturnsTrueWhenAChangeWasMade) // NOLINT
{
    std::string queryPackPath = "querypackpath";
    std::string queryPackPathDisabled = "querypackpath.DISABLED";

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(queryPackPathDisabled)).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(queryPackPathDisabled, queryPackPath));
    ASSERT_TRUE(Plugin::PluginUtils::enableQueryPack(queryPackPath));

    EXPECT_CALL(*mockFileSystem, exists(queryPackPathDisabled)).Times(1).WillRepeatedly(Return(false));
    ASSERT_FALSE(Plugin::PluginUtils::enableQueryPack(queryPackPath));
}

TEST_F(TestPluginUtilsWithMockFileSystem, testHaveCustomQueriesChanged)
{
    std::optional<std::string> value1 = "1";
    std::optional<std::string> value2 = "2";
    std::optional<std::string> emptyOptionalString;

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(value2.value()));
    // file exists, different value
    EXPECT_TRUE(Plugin::PluginUtils::haveCustomQueriesChanged(value1));
    // file exists, same value
    EXPECT_FALSE(Plugin::PluginUtils::haveCustomQueriesChanged(value2));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(2).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(2).WillRepeatedly(Return(false));
    // file doesn't exist, new custom pack has value
    EXPECT_TRUE(Plugin::PluginUtils::haveCustomQueriesChanged(value1));
    // file doesn't exist, new custom pack has no value
    EXPECT_FALSE(Plugin::PluginUtils::haveCustomQueriesChanged(emptyOptionalString));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(3).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(3).WillRepeatedly(Return(value2.value()));
    // DISABLED file exists, new custom pack has value
    EXPECT_TRUE(Plugin::PluginUtils::haveCustomQueriesChanged(value1));
    // DISABLED file exists, new custom pack has no value
    EXPECT_TRUE(Plugin::PluginUtils::haveCustomQueriesChanged(emptyOptionalString));
    // file doesn't exist, new custom pack has same value
    EXPECT_FALSE(Plugin::PluginUtils::haveCustomQueriesChanged(value2));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testDisableAllQueryPacks)
{
    std::string queryPackPath = "querypackpath";
    std::string queryPackPathDisabled = "querypackpath.DISABLED";

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryXDRConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryXDRConfigFilePath(), Plugin::osqueryXDRConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryMTRConfigFilePath(), Plugin::osqueryMTRConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryCustomConfigFilePath(), Plugin::osqueryCustomConfigFilePath()+".DISABLED"));
    Plugin::PluginUtils::disableAllQueryPacks();
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testIsQueryPackEnabled)
{
    std::string packName = "pack.conf";
    EXPECT_CALL(*mockFileSystem, isFile(packName)).WillOnce(Return(true));
    EXPECT_TRUE(Plugin::PluginUtils::isQueryPackEnabled(packName));

    EXPECT_CALL(*mockFileSystem, isFile(packName)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(packName+".DISABLED")).WillOnce(Return(true));
    EXPECT_FALSE(Plugin::PluginUtils::isQueryPackEnabled(packName));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testHandleDisablingAndEnablingScheduledQueryPacksDisablesAllWhenDataLimitHit)
{
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryXDRConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryXDRConfigFilePath(), Plugin::osqueryXDRConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryMTRConfigFilePath(), Plugin::osqueryMTRConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryCustomConfigFilePath(), Plugin::osqueryCustomConfigFilePath()+".DISABLED"));
    EXPECT_TRUE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({}, true));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testHandleDisablingAndEnablingScheduledQueryPacksEnablesAllWhenAllAreDisabled)
{
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath())).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath()+".DISABLED")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryXDRConfigFilePath()+".DISABLED")).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryXDRConfigFilePath()+".DISABLED", Plugin::osqueryXDRConfigFilePath()));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryMTRConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryMTRConfigFilePath()+".DISABLED", Plugin::osqueryMTRConfigFilePath()));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryCustomConfigFilePath()+".DISABLED", Plugin::osqueryCustomConfigFilePath()));

    EXPECT_TRUE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({"XDR", "MTR"}, false));
}


TEST_F(TestPluginUtilsWithStrictMockFileSystem, testHandleDisablingAndEnablingScheduledQueryPacksEnablesSomeWhenSomeAreDisabled)
{
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath())).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryMTRConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryMTRConfigFilePath()+".DISABLED", Plugin::osqueryMTRConfigFilePath()));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryCustomConfigFilePath()+".DISABLED", Plugin::osqueryCustomConfigFilePath()));

    EXPECT_TRUE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({"XDR", "MTR"}, false));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testHandleDisablingAndEnablingScheduledQueryPacksReturnsFalseWhenNoFilesNeedToBeMoved)
{
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath())).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath()+".DISABLED")).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(false));
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(false));

    EXPECT_FALSE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({}, false));


    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath())).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(false));

    EXPECT_FALSE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({"XDR", "MTR"}, false));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testHandleDisablingAndEnablingScheduledQueryPacksReturnsTrueWhenOnlyCustomPackNeedsToBeEnabled)
{
    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryXDRConfigFilePath())).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*mockFileSystem, isFile(Plugin::osqueryMTRConfigFilePath())).Times(1).WillRepeatedly(Return(true));

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, moveFile(Plugin::osqueryCustomConfigFilePath()+".DISABLED", Plugin::osqueryCustomConfigFilePath()));

    EXPECT_TRUE(Plugin::PluginUtils::handleDisablingAndEnablingScheduledQueryPacks({"XDR", "MTR"}, false));
}

TEST_F(TestPluginUtilsWithStrictMockFileSystem, testEnableCustomQueries)
{
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::osqueryCustomConfigFilePath(), "content"));

    bool restartNeeded = false;
    Plugin::PluginUtils::enableCustomQueries("content", restartNeeded, false);
    EXPECT_TRUE(restartNeeded);

    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath())).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()));
    EXPECT_CALL(*mockFileSystem, exists(Plugin::osqueryCustomConfigFilePath()+".DISABLED")).Times(1).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFileOrDirectory(Plugin::osqueryCustomConfigFilePath()+".DISABLED"));
    EXPECT_CALL(*mockFileSystem, writeFile(Plugin::osqueryCustomConfigFilePath()+".DISABLED", "content"));

    bool restartNeeded2 = false;
    Plugin::PluginUtils::enableCustomQueries("content", restartNeeded2, true);
    EXPECT_TRUE(restartNeeded2);
}