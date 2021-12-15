/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/Helpers/TempDir.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <modules/pluginimpl/OsqueryDataManager.h>

#include <gtest/gtest.h>

class TestOsqueryDataManager : public LogOffInitializedTests
{
protected:
    std::string createExcpetedOSqueryConfString(const std::string timeInSeconds)
    {
        std::stringstream content;
        content << R"(
        {
            "options": {
                "events_expiry": @@expiry@@,
                "syslog_events_expiry": @@expiry@@
            }
        })";
        return Common::UtilityImpl::StringUtils::replaceAll(
            content.str(), "@@expiry@@", timeInSeconds);

    }
};

TEST_F(TestOsqueryDataManager, rotateFileWorks) // NOLINT
{
    OsqueryDataManager m_DataManager;
    auto* ifileSystem = Common::FileSystem::fileSystem();
    Tests::TempDir tempdir { "/tmp" };

    std::string basefile = tempdir.dirPath() + "/file";

    ifileSystem->writeFileAtomically(basefile + ".1", "blah1", "/tmp");
    ifileSystem->writeFileAtomically(basefile + ".2", "blah2", "/tmp");
    m_DataManager.rotateFiles(basefile, 2);

    ASSERT_TRUE(ifileSystem->isFile(basefile + ".2"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".2"), "blah1");
    ASSERT_TRUE(ifileSystem->isFile(basefile + ".3"));
    ASSERT_EQ(ifileSystem->readFile(basefile + ".3"), "blah2");
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetCorrectlyWhenOldRecordTimeReturned) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});

    std::string timeString("10000");
    std::string expectedContent = createExcpetedOSqueryConfString("6000");

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters(10000, 4000);
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenOldRecordTimeDefautReturned) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});

    std::string timeString("604800");
    std::string expectedContent = createExcpetedOSqueryConfString("604800");

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters(604801, 604800);
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenOldRecordTimeIsGreaterThanCurrentTime) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});

    std::string timeString("604800");
    std::string expectedContent = createExcpetedOSqueryConfString("604800");

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));

    // simulate oldest time returned by query.
    // ensure values which would result in a negative outcome, actually result in default time being set, and no throws.
    EXPECT_NO_THROW(dataManager.reconfigureDataRetentionParameters(1000, 5000));
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenTimeCompareIsMoreThanSevenDays) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});

    std::string timeString("604800");
    std::string expectedContent = createExcpetedOSqueryConfString("604800");

    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters((604800 * 4), 604801);
}

TEST_F(TestOsqueryDataManager, getOldestAllowedTimeForCurre_ThrowsOnFailure) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
    EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(false));
    EXPECT_THROW(dataManager.getOldestAllowedTimeForCurrentEventedData(), std::runtime_error);
}

TEST_F(TestOsqueryDataManager, purgeDatabase_willRemoveAllFilesInDirectory) // NOLINT
{
    OsqueryDataManager dataManager;
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>{mockFileSystem});
    std::vector<std::string> filelist{"hello", "world"};
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(filelist));
    EXPECT_CALL(*mockFileSystem, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(filelist[0])).Times(1);
    EXPECT_CALL(*mockFileSystem, removeFile(filelist[1])).Times(1);
    dataManager.purgeDatabase();
}

