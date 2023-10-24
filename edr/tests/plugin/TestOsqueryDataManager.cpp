// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "pluginimpl/OsqueryDataManager.h"

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "tests/Common/Helpers/MockFileSystem.h"
#include "tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#include "Common/Helpers/TempDir.h"
#endif


#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/StringUtils.h"

#include <gtest/gtest.h>

#include <memory>

using namespace ::testing;

class TestOsqueryDataManager : public LogOffInitializedTests
{
public:
    void TearDown() override
    {
        Tests::restoreFileSystem();
    }

    std::string createExpectedOSqueryConfString(const std::string timeInSeconds)
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

    std::string timeString("10000");
    std::string expectedContent = createExpectedOSqueryConfString("6000");

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters(10000, 4000);
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenOldRecordTimeDefautReturned) // NOLINT
{
    OsqueryDataManager dataManager;

    std::string timeString("604800");
    std::string expectedContent = createExpectedOSqueryConfString("604800");

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters(604801, 604800);
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenOldRecordTimeIsGreaterThanCurrentTime) // NOLINT
{
    OsqueryDataManager dataManager;

    std::string timeString("604800");
    std::string expectedContent = createExpectedOSqueryConfString("604800");

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    // simulate oldest time returned by query.
    // ensure values which would result in a negative outcome, actually result in default time being set, and no throws.
    EXPECT_NO_THROW(dataManager.reconfigureDataRetentionParameters(1000, 5000));
}

TEST_F(TestOsqueryDataManager, reconfiguringForDataRetention_SetToDefaultCorrectlyWhenTimeCompareIsMoreThanSevenDays) // NOLINT
{
    OsqueryDataManager dataManager;

    std::string timeString("604800");
    std::string expectedContent = createExpectedOSqueryConfString("604800");

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, isFile(_)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, writeFile(_, expectedContent));
    Tests::replaceFileSystem(std::move(mockFileSystem));

    // simulate oldest time returned by query.
    dataManager.reconfigureDataRetentionParameters((604800 * 4), 604801);
}

TEST_F(TestOsqueryDataManager, getOldestAllowedTimeForCurre_ThrowsOnFailure) // NOLINT
{
    OsqueryDataManager dataManager;

    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    EXPECT_CALL(*mockFileSystem, exists(_)).WillOnce(Return(false));
    Tests::replaceFileSystem(std::move(mockFileSystem));
    EXPECT_THROW(dataManager.getOldestAllowedTimeForCurrentEventedData(), std::runtime_error);
}

TEST_F(TestOsqueryDataManager, purgeDatabase_willRemoveAllFilesInDirectory) // NOLINT
{
    OsqueryDataManager dataManager;
    auto mockFileSystem = std::make_unique<StrictMock<MockFileSystem>>();
    std::vector<std::string> filelist{"hello", "world"};
    EXPECT_CALL(*mockFileSystem, listFiles(_)).WillOnce(Return(filelist));
    EXPECT_CALL(*mockFileSystem, isDirectory(_)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, removeFile(filelist[0])).Times(1);
    EXPECT_CALL(*mockFileSystem, removeFile(filelist[1])).Times(1);
    Tests::replaceFileSystem(std::move(mockFileSystem));
    dataManager.purgeDatabase();
}

TEST_F(TestOsqueryDataManager, buildLimitQueryString_returnsCorrectQueryString) // NOLINT
{
    OsqueryDataManager dataManager;
    std::string query = dataManager.buildLimitQueryString(123);

    // Left text alignment needed for test
    std::string expectedQuery =
R"(WITH time_values AS (
SELECT (SELECT time FROM process_events ORDER BY time DESC LIMIT 1 OFFSET 123) AS oldest_time
union
SELECT(SELECT time FROM user_events ORDER BY time DESC LIMIT 1 OFFSET 123) AS oldest_time
union
SELECT(SELECT time FROM selinux_events ORDER BY time DESC LIMIT 1 OFFSET 123) AS oldest_time
union
SELECT(SELECT time FROM socket_events ORDER BY time DESC LIMIT 1 OFFSET 123) AS oldest_time
union
SELECT(SELECT time FROM syslog_events ORDER BY time DESC LIMIT 1 OFFSET 123) AS oldest_time
)
SELECT MIN(oldest_time) AS time_to_keep FROM time_values WHERE oldest_time > 0)";
    ASSERT_EQ(query, expectedQuery);
}

TEST_F(TestOsqueryDataManager, buildLimitQueryString_returnsCorrectQueryStringUnderBound) // NOLINT
{
    OsqueryDataManager dataManager;
    std::string query = dataManager.buildLimitQueryString(0);

    // Left text alignment needed for test
    std::string expectedQuery =
        R"(WITH time_values AS (
SELECT (SELECT time FROM process_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM user_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM selinux_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM socket_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM syslog_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
)
SELECT MIN(oldest_time) AS time_to_keep FROM time_values WHERE oldest_time > 0)";
    ASSERT_EQ(query, expectedQuery);
}

TEST_F(TestOsqueryDataManager, buildLimitQueryString_returnsCorrectQueryStringOverBound) // NOLINT
{
    OsqueryDataManager dataManager;

    // 5 million events is above current limit.
    std::string query = dataManager.buildLimitQueryString(5000000);

    // Left text alignment needed for test
    std::string expectedQuery =
        R"(WITH time_values AS (
SELECT (SELECT time FROM process_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM user_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM selinux_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM socket_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
union
SELECT(SELECT time FROM syslog_events ORDER BY time DESC LIMIT 1 OFFSET 100000) AS oldest_time
)
SELECT MIN(oldest_time) AS time_to_keep FROM time_values WHERE oldest_time > 0)";
    ASSERT_EQ(query, expectedQuery);
}
