// Copyright 2019-2023 Sophos Limited. All rights reserved.

#ifdef SPL_BAZEL
#include "tests/Common/Helpers/LogInitializedTests.h"
#include "tests/Common/Helpers/TempDir.h"
#else
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/TempDir.h"
#endif

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Process/IProcess.h"
#include "livequery/Logger.h"
#include "osqueryclient/OsqueryProcessor.h"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <future>
#include <thread>

#include <stdlib.h>
using namespace ::testing;
//#ifndef OSQUERYBIN
//#    define OSOSQUERYBIN ""
//#endif

class TestOSQueryProcessor : public  LogOffInitializedTests
{
public:
    Tests::TempDir m_tempDir;
    Common::Process::IProcessPtr m_osqueryProcess;

    TestOSQueryProcessor() : m_tempDir("/tmp")
    {
        if (skipTest())
            return;
        m_osqueryProcess = startOsquery();
        std::cout << "osquery pid: " << m_osqueryProcess->childPid() << std::endl;
    }

    std::string osqueryBinaryPath()
    {
        char* OSQ_ENV_PATH = secure_getenv("OVERRIDE_OSQUERY_BIN");
        if (OSQ_ENV_PATH)
        {
            return std::string { OSQ_ENV_PATH };
        }
//        std::string COMPILED_DEFINED_PATH { OSQUERYBIN };
//        if (Common::FileSystem::fileSystem()->exists(COMPILED_DEFINED_PATH))
//        {
//            return COMPILED_DEFINED_PATH;
//        }
        throw std::logic_error("Osquery path not found. Please, set environment variable OVERRIDE_OSQUERY_BIN");
    }

    bool skipTest()
    {
        return secure_getenv("RUN_GOOGLE_COMPONENT_TESTS") == nullptr;
    }

    Common::Process::IProcessPtr startOsquery()
    {
        std::string osqueryBin = osqueryBinaryPath();

        auto osqu = Common::Process::createProcess();
        std::vector<std::string> flags { { "--ephemeral" },
                                         { "--extensions_socket" },
                                         { osquerySocket() },
                                         { "--logger_path" },
                                         { m_tempDir.dirPath() },
                                         { "--D" },
                                         { "--watchdog_memory_limit" },
                                         { "100" },
                                         { "--watchdog_utilization_limit" },
                                         { "10" },
                                         { "--verbose" } };
        osqu->exec(osqueryBin, flags);
        return osqu;
    }

    std::string osquerySocket()
    {
        return m_tempDir.absPath("osquery.sock");
    }

    ::testing::AssertionResult responseIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const livequery::QueryResponse& actualResponse,
        const livequery::QueryResponse& expectedResponse);

    ~TestOSQueryProcessor()
    {
        if (m_osqueryProcess)
        {
            m_osqueryProcess->kill();
        }
    }
    livequery::QueryResponse success(
        livequery::ResponseData::ColumnHeaders headers,
        livequery::ResponseData::ColumnData columnData)
    {
        livequery::ResponseStatus status { livequery::ErrorCode::SUCCESS };

        livequery::QueryResponse response { status, livequery::ResponseData { headers, columnData }, livequery::ResponseMetaData() };

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);
        return response;
    }

    livequery::QueryResponse failure(livequery::ErrorCode errorCode, std::string description)
    {
        livequery::ResponseStatus status { errorCode };
        status.overrideErrorDescription(description);

        livequery::QueryResponse response { status, livequery::ResponseData::emptyResponse(), livequery::ResponseMetaData() };

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);
        return response;
    }
};
::testing::AssertionResult TestOSQueryProcessor::responseIsEquivalent(
    const char* m_expr,
    const char* n_expr,
    const livequery::QueryResponse& actualResponse,
    const livequery::QueryResponse& expectedResponse)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";
    if (expectedResponse.status().errorCode() != actualResponse.status().errorCode())
    {
        return ::testing::AssertionFailure()
               << s.str() << " Expected response status : " << expectedResponse.status().errorValue()
               << ". Actual response status: " << actualResponse.status().errorValue() << ".";
    }
    std::string actualDescription = actualResponse.status().errorDescription();
    std::string expectedPartOfDescription = expectedResponse.status().errorDescription();

    if (actualDescription.find(expectedPartOfDescription) == std::string::npos)
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response status error differ. "
                                             << expectedPartOfDescription << " Not inside the: " << actualDescription;
    }

    if (expectedResponse.data().columnData() != actualResponse.data().columnData())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response column data";
    }

    if (expectedResponse.data().columnHeaders() != actualResponse.data().columnHeaders())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response column data";
    }

    return ::testing::AssertionSuccess();
}

TEST_F(TestOSQueryProcessor, VerifyOsqueryCanBeStarted) // NOLINT
{
    if (skipTest())
        return;
    std::string osqueryLog { m_tempDir.absPath("osqueryd.INFO") };
    for (int i = 1; i < 15; i++)
    {
        if (Common::FileSystem::fileSystem()->exists(osqueryLog))
        {
            std::string logContent = Common::FileSystem::fileSystem()->readFile(osqueryLog);
            if (logContent.find("Event publisher not enabled") != std::string::npos)
            {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    m_osqueryProcess->kill();
    std::string output = m_osqueryProcess->output();
    EXPECT_THAT(output, ::testing::HasSubstr("Extension manager service starting: /tmp/"));
}

TEST_F(TestOSQueryProcessor, SimpleSelectShouldReturn) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select name,value from osquery_flags where name=='ephemeral'");
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back({ { "name", "ephemeral" }, { "value", "true" } });
    auto expectedResponse = success(
        { { "name", OsquerySDK::ColumnType::TEXT_TYPE },
          { "value", OsquerySDK::ColumnType::TEXT_TYPE } },
        columnData);
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, NoOSqueryAvailableShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    m_osqueryProcess->kill();
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("foo");
    auto expectedResponse = failure(livequery::ErrorCode::UNEXPECTEDERROR, "connect");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, InvalidStatementShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("foo");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "syntax error");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, InvalidTableNameShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select * from foo");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "no such table");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, InvalidColumnNameShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select invalidname from osquery_flags");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "no such column");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, AttemptToModifyAnOsQueryTableShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("insert into processes values(path='foo')");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "may not be modified");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, CreatingTempTableThatAlreadyExistsShouldReturnError) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("create table myprocesses(bar int); create table myprocesses(hello int)");
    auto expectedResponse = failure(livequery::ErrorCode::OSQUERYERROR, "already exist");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, LiveQueryShouldReportOnQueriesThatCausedOsqueryWatchdogToRestartOsquery) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    std::string insaneQuery { R"(WITH RECURSIVE
counting (curr, next)
AS
( SELECT 1,1
  UNION ALL
  SELECT next, curr+1 FROM counting
  LIMIT 10000000000 )
SELECT group_concat(curr) FROM counting;)" };
    auto response = osqueryProcessor.query(insaneQuery);
    auto expectedResponse = failure(livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING, "running query");
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}

TEST_F(TestOSQueryProcessor, VerifyOSQueryResponseHasExpectedTypesForINTEGERandBIGINT) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    auto response = osqueryProcessor.query("select core,user from cpu_time Limit 1");
    livequery::ResponseData::ColumnHeaders expectedHeaders;
    expectedHeaders.push_back({ "core", OsquerySDK::ColumnType::INTEGER_TYPE });
    expectedHeaders.push_back({ "user", OsquerySDK::ColumnType::BIGINT_TYPE});
    EXPECT_EQ(expectedHeaders, response.data().columnHeaders());
}

TEST_F(TestOSQueryProcessor, HeaderWillBeDeducedIfNotCompletellyGivenByOsQueryAndTheTypeWillAlwaysBeText) // NOLINT
{
    if (skipTest())
        return;
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    // osquery.getQueryColumns will only return information for user and type BIGINT
    // the information from core will be 'deduced'.
    auto response = osqueryProcessor.query("select user from cpu_time Limit 1;select core from cpu_time Limit 1");
    auto& responseColumnData = response.data().columnData();

    livequery::ResponseData::ColumnData columnData;
    // I want to verify only the structure and the headers, not the value.
    columnData.push_back({ { "user", responseColumnData.at(0).at("user") } });
    columnData.push_back({ { "core", responseColumnData.at(1).at("core") } });
    livequery::ResponseData::ColumnHeaders headers {
        { "user", OsquerySDK::ColumnType::BIGINT_TYPE },
        { "core", OsquerySDK::ColumnType::TEXT_TYPE }, // core was deduced from the columnData
    };
    auto expectedResponse = success(headers, columnData);
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}
