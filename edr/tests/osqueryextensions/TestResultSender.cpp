/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFilePermissions.h>
#include <Common/Helpers/MockFileSystem.h>
#include <modules/livequery/ResponseData.h>
#include <modules/osqueryextensions/ResultsSender.h>

#include <gtest/gtest.h>

using namespace ::testing;

const std::string INTERMEDIARY_PATH = "intermediary";
const std::string DATAFEED_PATH = "datafeed";
const std::string QUERY_PACK_PATH = "querypack";
const std::string EMPTY_QUERY_PACK = "{}";

const std::string EXAMPLE_QUERY_PACK =  R"({
    "schedule": {
        "deb_packages": {
            "query": "SELECT name, version, arch, revision FROM deb_packages;",
            "interval": 14400,
            "removed": false,
            "blacklist": true,
            "platform": "linux",
            "description": "Gets all the installed DEB packages in the target Linux system.",
            "tag": "DataLake"
        }
    },
    "packs": {
        "mtr": {
            "queries": {
                "osquery_rocksdb_size_linux": {
                    "query": "WITH files (\n  number_of_files,\n  total_size,\n  mb\n) AS (\n  SELECT count(*) AS number_of_files,\n  SUM(size) AS total_size,\n  SUM(size)/1024/1024 AS mb\n  FROM file\n  WHERE path LIKE '/opt/sophos-spl/plugins/mtr/dbos/data/osquery.db/%'\n)\nSELECT\n  number_of_files,\n  total_size,\n  mb\nFROM files\nWHERE mb > 50;",
                    "interval": 86400,
                    "removed": false,
                    "blacklist": false,
                    "platform": "linux",
                    "description": "Retrieves the size of Osquery RocksDB on Linux.",
                    "tag": "stream"
                }
            },
            "discovery": [
                "SELECT\n    name\nFROM\n    osquery_extensions\nWHERE\n    name = 'sophosmdrextension'"
            ]
        }
    },
    "decorators": {
        "interval": {
            "3600": [
                "SELECT endpoint_id AS eid from sophos_endpoint_info",
                "SELECT\n    interface_details.mac AS mac_address,\n    interface_addresses.mask AS ip_mask,\n    interface_addresses.address AS ip_address\nFROM\n    interface_addresses\n    JOIN interface_details ON interface_addresses.interface = interface_details.interface\nWHERE\n    ip_address NOT LIKE '127.%'\n    AND ip_address NOT LIKE '%:%'\n    AND ip_address NOT LIKE '169.254.%'\n    AND ip_address NOT LIKE '%.1'\nORDER BY\n    interface_details.last_change\nLIMIT\n    1",
                "SELECT\n    user AS username\nFROM\n    logged_in_users\nWHERE\n    (\n        type = 'user'\n        OR type = 'active'\n    )\nORDER BY\n    time DESC\nLIMIT\n    1"
            ]
        },
        "load": [
            "SELECT (unix_time - (select total_seconds from uptime)) AS boot_time FROM time",
            "SELECT\n    CASE\n        WHEN computer_name == '' THEN hostname\n        ELSE computer_name\n    END AS hostname\nFROM\n    system_info",
            "SELECT\n    name AS os_name,\n    version AS os_version,\n    platform AS os_platform\nFROM\n    os_version\nLIMIT\n    1",
            "SELECT\n    CASE \n        WHEN upper(platform) == 'WINDOWS' AND upper(name) LIKE '%SERVER%' THEN 'server' \n        WHEN upper(platform) == 'WINDOWS' AND upper(name) NOT LIKE '%SERVER%' THEN 'client' \n        WHEN upper(platform) == 'DARWIN' THEN 'client' \n        WHEN (SELECT count(*) FROM system_info WHERE cpu_brand LIKE '%Xeon%') == 1 THEN 'server' \n        WHEN (SELECT count(*) FROM system_info WHERE hardware_vendor LIKE '%VMWare%') == 1 THEN 'server' \n        WHEN (SELECT count(*) FROM system_info WHERE hardware_vendor LIKE '%QEMU%') == 1 THEN 'server' \n        WHEN (\n            (SELECT obytes FROM interface_details ORDER by obytes DESC LIMIT 1) > (SELECT ibytes FROM interface_details ORDER by ibytes DESC LIMIT 1)\n            ) == 1 THEN 'server'\n        ELSE 'client'\n    END AS 'os_type'\nFROM 'os_version';",
            "SELECT endpoint_id AS eid from sophos_endpoint_info",
            "SELECT\n    interface_details.mac AS mac_address,\n    interface_addresses.mask AS ip_mask,\n    interface_addresses.address AS ip_address\nFROM\n    interface_addresses\n    JOIN interface_details ON interface_addresses.interface = interface_details.interface\nWHERE\n    ip_address NOT LIKE '127.%'\n    AND ip_address NOT LIKE '%:%'\n    AND ip_address NOT LIKE '169.254.%'\n    AND ip_address NOT LIKE '%.1'\nORDER BY\n    interface_details.last_change\nLIMIT\n    1",
            "SELECT\n    user AS username\nFROM\n    logged_in_users\nWHERE\n    (\n        type = 'user'\n        OR type = 'active'\n    )\nORDER BY\n    time DESC\nLIMIT\n    1",
            "SELECT '1.1.19' query_pack_version"
        ]
    }
})";


class TestResultSender : public LogOffInitializedTests{};

class ResultSenderForUnitTests : public ResultsSender
{
public:
    ResultSenderForUnitTests(const std::string& intermediaryPath,
                             const std::string& datafeedPath,
                             const std::string& osqueryXDRConfigFilePath) :
        ResultsSender(intermediaryPath, datafeedPath, osqueryXDRConfigFilePath)
    {}

    std::vector<ScheduledQuery> getQueryTags()
    {
        return m_scheduledQueryTags;
    }

    std::map<std::string, std::pair<std::string, std::string>> getQueryTagMapOveridden()
    {
        return getQueryTagMap();
    }
};

TEST_F(TestResultSender, loadScheduledQueryTags) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EXAMPLE_QUERY_PACK));
    ResultSenderForUnitTests resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);

    auto actualQueries = resultsSender.getQueryTags();
    auto actualQueryTagMap = resultsSender.getQueryTagMapOveridden();

    std::vector<ScheduledQuery> expectedQueries;
    expectedQueries.push_back({"deb_packages", "deb_packages", "DataLake"});
    expectedQueries.push_back({"pack_mtr_osquery_rocksdb_size_linux", "osquery_rocksdb_size_linux", "stream"});

    std::map<std::string, std::pair<std::string, std::string>> tagMap;
    tagMap.insert(std::make_pair("deb_packages", std::make_pair("deb_packages", "DataLake")));
    tagMap.insert(std::make_pair("pack_mtr_osquery_rocksdb_size_linux", std::make_pair("osquery_rocksdb_size_linux", "stream")));

    ASSERT_EQ(actualQueries[0].queryNameWithPack, expectedQueries[0].queryNameWithPack);
    ASSERT_EQ(actualQueries[0].queryName, expectedQueries[0].queryName);
    ASSERT_EQ(actualQueries[0].tag, expectedQueries[0].tag);

    ASSERT_EQ(actualQueries[1].queryNameWithPack, expectedQueries[1].queryNameWithPack);
    ASSERT_EQ(actualQueries[1].queryName, expectedQueries[1].queryName);
    ASSERT_EQ(actualQueries[1].tag, expectedQueries[1].tag);

    ASSERT_EQ(actualQueryTagMap["deb_packages"].first, tagMap["deb_packages"].first);
    ASSERT_EQ(actualQueryTagMap["deb_packages"].second, tagMap["deb_packages"].second);

    ASSERT_EQ(actualQueryTagMap["pack_mtr_osquery_rocksdb_size_linux"].first, tagMap["pack_mtr_osquery_rocksdb_size_linux"].first);
    ASSERT_EQ(actualQueryTagMap["pack_mtr_osquery_rocksdb_size_linux"].second, tagMap["pack_mtr_osquery_rocksdb_size_linux"].second);
}

TEST_F(TestResultSender, resetRemovesExistingBatchFile) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    // Force false here so that when send is called in the constructor we skip sending, but then to check the reset
    // is working we expect true so that the file can be removed
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, removeFile(INTERMEDIARY_PATH)).Times(1);
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "[")).Times(1);
    resultsSender.Reset();
}

TEST_F(TestResultSender, addWritesToFile) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    std::string testResult = R"({"name":"","test":"value"})";
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, testResult)).Times(1);
    resultsSender.Add(testResult);
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false));
}

TEST_F(TestResultSender, addAppendsToFileExistinEntries) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    std::string testResultString1 = R"({"name":"","test":"value"})";
    std::string testResultString2 = R"({"name":"","test2":"value2"})";
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, testResultString1)).Times(1);
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "," + testResultString2)).Times(1);
    resultsSender.Add(testResultString1);
    resultsSender.Add(testResultString2);
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false));
}

TEST_F(TestResultSender, addThrowsInvalidJsonLog) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, appendFile(_, _)).Times(0);
    EXPECT_THROW(resultsSender.Add(R"(not json)"), std::exception);
}

TEST_F(TestResultSender, addThrowsWhenAppendThrows) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, appendFile(_, _)).WillOnce(Throw(std::runtime_error("TEST")));
    EXPECT_THROW(resultsSender.Add("{\"test\":\"value\"}"), std::runtime_error);
}

TEST_F(TestResultSender, getFileSizeQueriesFile) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    // First false to skip send in constructor, then true so that size is calculated (returns 0 if no file) and lastly false again so that the send in the destructor is skipped too.
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, fileSize(INTERMEDIARY_PATH)).WillOnce(Return(1));

    // 3 is due to the 2 added in GetFileSize() (for the , and ] that gets added) and the 1 from the above line mock here
    ASSERT_EQ(resultsSender.GetFileSize(), 3);
}

TEST_F(TestResultSender, getFileSizeZeroWhenFileDoesNotExist) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    // First false to skip send in constructor, then false so that size is not calculated (returns 0 if no file) and lastly false again so that the send in the destructor is skipped too.
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    ASSERT_EQ(resultsSender.GetFileSize(), 0);
}

TEST_F(TestResultSender, getFileSizePropagatesException) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, fileSize(INTERMEDIARY_PATH)).WillOnce(Throw(std::runtime_error("TEST")));
    EXPECT_THROW(resultsSender.GetFileSize(), std::runtime_error);
}

TEST_F(TestResultSender, sendMovesBatchFile) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    auto mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
    Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    EXPECT_CALL(*mockFilePermissions, chown(INTERMEDIARY_PATH, "sophos-spl-local", "sophos-spl-group"));

    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);

    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "]")).Times(1);
    EXPECT_CALL(*mockFileSystem, moveFile(INTERMEDIARY_PATH, StartsWith(DATAFEED_PATH + "/scheduled_query"))).Times(1);
    resultsSender.Send();
}

TEST_F(TestResultSender, SendDoesNotMoveNoBatchFileIfItDoesNotExist) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, moveFile(_, _)).Times(0);

    resultsSender.Send();
}

TEST_F(TestResultSender, sendThrowsWhenFileMoveThrows) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    auto mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
    Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    EXPECT_CALL(*mockFilePermissions, chown(INTERMEDIARY_PATH, "sophos-spl-local", "sophos-spl-group"));

    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "]")).Times(1);
    EXPECT_CALL(*mockFileSystem, moveFile(INTERMEDIARY_PATH, StartsWith(DATAFEED_PATH + "/scheduled_query")))
        .WillOnce(Throw(std::runtime_error("TEST")));

    EXPECT_THROW(resultsSender.Send(), std::runtime_error);
}

TEST_F(TestResultSender, sendIfFileExistsAtStart) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    auto mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
    Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    EXPECT_CALL(*mockFilePermissions, chown(INTERMEDIARY_PATH, "sophos-spl-local", "sophos-spl-group"));
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(true)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "]")).Times(1);
    EXPECT_CALL(*mockFileSystem, moveFile(INTERMEDIARY_PATH, StartsWith(DATAFEED_PATH + "/scheduled_query"))).Times(1);
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);

}

TEST_F(TestResultSender, sendIfFileExistsAtShutdown) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    auto mockFilePermissions = new ::testing::NiceMock<MockFilePermissions>();
    Tests::replaceFilePermissions(std::unique_ptr<Common::FileSystem::IFilePermissions>{mockFilePermissions});
    EXPECT_CALL(*mockFilePermissions, chown(INTERMEDIARY_PATH, "sophos-spl-local", "sophos-spl-group"));
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, "]")).Times(1);
    EXPECT_CALL(*mockFileSystem, moveFile(INTERMEDIARY_PATH, StartsWith(DATAFEED_PATH + "/scheduled_query"))).Times(1);
    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
}

TEST_F(TestResultSender, FirstAddFailureDoesNotAddCommaNext) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EMPTY_QUERY_PACK));

    ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    std::string testJsonResult = R"({"name":"","test":"value"})";
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, testJsonResult))
        .Times(2)
        .WillOnce(Throw(std::runtime_error("TEST")))
        .WillOnce(Return());
    EXPECT_THROW(resultsSender.Add(testJsonResult), std::runtime_error);
    resultsSender.Add(testJsonResult);
}

TEST_F(TestResultSender, testQueryNameCorrectedFromQueryPackMap) // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH)).WillOnce(Return(false)).WillOnce(Return(false));
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EXAMPLE_QUERY_PACK));
    ResultSenderForUnitTests resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
    std::string testResult = R"({"name":"pack_mtr_osquery_rocksdb_size_linux"})";
    std::string correctedNameResult = "{\"name\":\"osquery_rocksdb_size_linux\",\"tag\":\"stream\"}";
    EXPECT_CALL(*mockFileSystem, appendFile(INTERMEDIARY_PATH, correctedNameResult)).Times(1);
    resultsSender.Add(testResult);
}

TEST_F(TestResultSender, initialExistsThrowsExceptionContinuesConstructor)  // NOLINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem, readFile(QUERY_PACK_PATH)).WillOnce(Return(EXAMPLE_QUERY_PACK));
    EXPECT_CALL(*mockFileSystem, exists(INTERMEDIARY_PATH))
        .WillOnce(Throw(std::runtime_error("TEST")))
        .WillOnce(Return(false));
  ResultsSender resultsSender(INTERMEDIARY_PATH, DATAFEED_PATH, QUERY_PACK_PATH);
}
