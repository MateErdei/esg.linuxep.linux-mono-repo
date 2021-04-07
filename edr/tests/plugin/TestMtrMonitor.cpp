/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/FileSystemReplaceAndRestore.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Helpers/MockFileSystem.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <modules/pluginimpl/MtrMonitor.h>
#include <tests/osqueryclient/MockOsqueryClient.h>

#include <gtest/gtest.h>

class TestableMtrMonitor : public MtrMonitor
{
    public:
    explicit TestableMtrMonitor(std::unique_ptr<osqueryclient::IOsqueryClient> osqueryClient)
    : MtrMonitor(std::move(osqueryClient))
    {
    }
        std::optional<std::string> processFlagList(const std::vector<std::string>& flags, const std::string& flagToFind)
        {
            return MtrMonitor::processFlagList(flags, flagToFind);
        }

        std::optional<std::string> findMtrSocketPath()
        {
            return MtrMonitor::findMtrSocketPath();
        }
};

std::string EXAMPLE_FLAGS_FILE_CONTENTS="--host_identifier=uuid\n"
    "--pidfile=/opt/sophos-spl/plugins/mtr/dbos/data/osquery.pid\n"
    "--database_path=/opt/sophos-spl/plugins/mtr/dbos/data/osquery.db\n"
    "--log_result_events=true\n"
    "--force\n"
    "--utc\n"
    "--allow_unsafe\n"
    "--enroll_secret_env=DB_ENROLL_SECRET\n"
    "--enroll_tls_endpoint=/api/enroll\n"
    "--tls_hostname=prod.endpointintel.darkbytes.io:443\n"
    "--tls_server_certs=/opt/sophos-spl/plugins/mtr/dbos/data/certificate.crt\n"
    "--config_plugin=tls\n"
    "--config_tls_endpoint=/api/config\n"
    "--config_tls_refresh=14400\n"
    "--disable_extensions=false\n"
    "--extensions_socket=/opt/sophos-spl/plugins/mtr/dbos/data/osquery_NljewDlNWB.sock\n"
    "--distributed_plugin=tls\n"
    "--disable_distributed=false\n"
    "--distributed_interval=30\n"
    "--distributed_tls_read_endpoint=/api/distributed/read\n"
    "--distributed_tls_write_endpoint=/api/distributed/write\n"
    "--distributed_tls_max_attempts=12\n"
    "--logger_stderr=true\n"
    "--logger_tls_compress=true\n"
    "--logger_path=/opt/sophos-spl/plugins/mtr/dbos/data/logs\n"
    "--logger_mode=420\n"
    "--augeas_lenses=/opt/sophos-spl/plugins/mtr/osquery/share/lenses\n"
    "--logger_min_stderr=1\n"
    "--logger_min_status=1\n"
    "--disable_watchdog=false\n"
    "--watchdog_level=0\n"
    "--watchdog_memory_limit=250\n"
    "--watchdog_utilization_limit=30\n"
    "--watchdog_delay=60\n"
    "--enable_extensions_watchdog=true";


class TestMtrMonitor : public LogOffInitializedTests{};

TEST_F(TestMtrMonitor, processFlagListReturnsNullIfFlagsEmptyString) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {""};
    std::string flagToFind = "exampleFlag";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_FALSE(result.has_value());
}

TEST_F(TestMtrMonitor, processFlagListReturnsNullIfFlagsEmptyVector) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {};
    std::string flagToFind = "exampleFlag";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_FALSE(result.has_value());
}

TEST_F(TestMtrMonitor, processFlagListReturnsFlagValueFromFile) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"--config_tls_refresh=14400", "--extraFlag=itdoesntmatter"};
    std::string flagToFind = "config_tls_refresh";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "14400");
}

TEST_F(TestMtrMonitor, processFlagListReturnsFlagValueFromRealFlagsFile) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = Common::UtilityImpl::StringUtils::splitString(EXAMPLE_FLAGS_FILE_CONTENTS, "\n");
    std::string flagToFind = "config_tls_refresh";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "14400");
}

TEST_F(TestMtrMonitor, processFlagListReturnsNullWhenFlagToFindIncludesHyphens) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = Common::UtilityImpl::StringUtils::splitString(EXAMPLE_FLAGS_FILE_CONTENTS, "\n");
    std::string flagToFind = "--config_tls_refresh";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_FALSE(result.has_value());
}

TEST_F(TestMtrMonitor, processFlagListReturnsNullFromRealFlagsFileMissingTheFlagName) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = Common::UtilityImpl::StringUtils::splitString(EXAMPLE_FLAGS_FILE_CONTENTS, "\n");
    std::string flagToFind = "made_up_flag_name";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_FALSE(result.has_value());
}

TEST_F(TestMtrMonitor, processFlagListReturnsNullWhenFlagsFileIsMissingEqualsSeparator) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"--validFlagFlagValueWithNoEquals"};
    std::string flagToFind = "validFlag";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_FALSE(result.has_value());
}

TEST_F(TestMtrMonitor, processFlagListReturnsEmptyStringWhenFlagsFileIsMissingFlagValue) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"--validFlag="};
    std::string flagToFind = "validFlag";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "");
}

TEST_F(TestMtrMonitor, processFlagListReturnsLastValueAndDoesNotThrowIfMultipleEqualsInFlagValue) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"--validFlag=A=B=C"};
    std::string flagToFind = "validFlag";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "A=B=C");
}

TEST_F(TestMtrMonitor, processFlagListReturnsFlagValueFromFlagsFileWithLeadingWhitespace) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"   --config_tls_refresh=balvenie"};
    std::string flagToFind = "config_tls_refresh";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "balvenie");
}

TEST_F(TestMtrMonitor, processFlagListReturnsFlagValueFromFlagsFileWithUnusualSocketPath) //NO_LINT
{
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    std::vector<std::string> flags = {"   --apath=/opt/so phos/--socket=/=4f/r=--apath=socket"};
    std::string flagToFind = "apath";
    auto result = mtrMonitor.processFlagList(flags, flagToFind);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), "/opt/so phos/--socket=/=4f/r=--apath=socket");
}

TEST_F(TestMtrMonitor, findMtrSocketPathReturnsFlagValueFromFlagsFile) //NO_LINT
{
    std::vector<std::string> flags = {"some text", "--extensions_socket=/socket/path/socket.sock", "--flag=value"};
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem,
        exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
        readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_EQ(mtrMonitor.findMtrSocketPath(), "/socket/path/socket.sock");
}

TEST_F(TestMtrMonitor, findMtrSocketPathDefaultsToNotSetIfFileMissing) //NO_LINT
{
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });
    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(false));

    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_EQ(mtrMonitor.findMtrSocketPath(), std::nullopt);
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredReturnsTrueByDefault) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(Return(OsquerySDK::Status{0,"some message"}));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_TRUE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredReturnsTrueIfQueryReturnsCountGreaterThanFive) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "123";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_TRUE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredReturnsTrueIfQueryReturnsCountOfFive) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "5";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_TRUE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredReturnsTrueIfQueryReturnsZeroCount) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "0";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_FALSE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredReturnsTrueIfQueryReturnsLowNonZeroCount) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["query_count"] = "3";
    OsquerySDK::QueryData data;
    data.push_back(row);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_FALSE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesConfiguredDefaultsToTrueAndIgnoresUnexpectedData) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["a"] = "2";
    row["b"] = "sometextthatdoesn'tmatter123";
    row["c"] = "123";
    OsquerySDK::TableRow row2;
    row2["a2"] = "2";
    row2["b2"] = "sometextthatdoesn'tmatter123";
    row2["c2"] = "123";
    OsquerySDK::QueryData data;
    data.push_back(row);
    data.push_back(row2);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_TRUE(mtrMonitor.hasScheduledQueriesConfigured());
}

TEST_F(TestMtrMonitor, hasScheduledQueriesReturnsTrueAndIgnoresUnexpectedDataEvenWhenCorrectFieldPresent) //NO_LINT
{
    std::string socket = "/socket/path/socket.sock";
    std::unique_ptr<MockIOsqueryClient> mockOsqueryClient = std::make_unique<MockIOsqueryClient>();
    EXPECT_CALL(*mockOsqueryClient, connect(socket)).Times(1);

    OsquerySDK::TableRow row;
    row["a"] = "2";
    row["b"] = "";
    row["c"] = "123";
    OsquerySDK::TableRow row2;
    row2["a2"] = "2";
    row2["query_count"] = "0";
    row2["c2"] = "123";
    OsquerySDK::QueryData data;
    data.push_back(row);
    data.push_back(row2);
    EXPECT_CALL(*mockOsqueryClient, query(_,_)).WillOnce(
        DoAll(SetArgReferee<1>(data), Return(OsquerySDK::Status{0,"some message"})));

    auto mockFileSystem = new ::testing::StrictMock<MockFileSystem>();
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem> { mockFileSystem });

    EXPECT_CALL(*mockFileSystem,
                exists("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(true));

    std::vector<std::string> flags = {"some text", "--extensions_socket="+socket, "--flag=value"};
    EXPECT_CALL(*mockFileSystem,
                readLines("/opt/sophos-spl/plugins/mtr/dbos/data/osquery.flags")).WillOnce(Return(flags));
    EXPECT_CALL(*mockFileSystem,
                exists("/socket/path/socket.sock")).WillOnce(Return(true));

    TestableMtrMonitor mtrMonitor(std::move(mockOsqueryClient));
    ASSERT_TRUE(mtrMonitor.hasScheduledQueriesConfigured());
}
