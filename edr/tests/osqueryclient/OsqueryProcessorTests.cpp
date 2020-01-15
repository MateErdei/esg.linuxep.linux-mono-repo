/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/FileSystem/IFileSystem.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <modules/livequery/Logger.h>
#include <modules/osqueryclient/OsqueryProcessor.h>
#include <Common/Process/IProcess.h>
#include <Common/Helpers/TempDir.h>
#include <thread>
#include <future>

using namespace ::testing;
#ifndef OSQUERYBIN
#define OSOSQUERYBIN ""
#endif

class TestOSQueryProcessor : public ::testing::Test
{
public:
    Tests::TempDir m_tempDir;
    Common::Process::IProcessPtr m_osqueryProcess;

    TestOSQueryProcessor() : m_tempDir("/tmp")
    {
        m_osqueryProcess = startOsquery();
        std::cout << "osquery pid: " << m_osqueryProcess->childPid() << std::endl;
    }

    Common::Process::IProcessPtr startOsquery()
    {
        std::string osqueryBin{OSQUERYBIN};
        assert(osqueryBin != "");

        auto osqu = Common::Process::createProcess();
        std::vector<std::string> flags{
                {"--ephemeral"},
                {"--extensions_socket"},
                {osquerySocket()},
                {"--D"},
                {"--disable_logging=True"}
        };
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
            const livequery::QueryResponse& expectedResponse,
            const livequery::QueryResponse& actualResponse);

    ~TestOSQueryProcessor()
    {
        m_osqueryProcess->kill();

    }
    livequery::QueryResponse success(livequery::ResponseData::ColumnHeaders headers, livequery::ResponseData::ColumnData columnData)
    {
        livequery::ResponseStatus status{ livequery::ErrorCode::SUCCESS};

        livequery::QueryResponse response{status, livequery::ResponseData{headers,columnData}};

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);
        return response;
    }

};

TEST_F(TestOSQueryProcessor, SimpleSelectShouldReturn) // NOLINT
{
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));//FIXME remove this sleep
    auto response = osqueryProcessor.query("select name,value from osquery_flags where name=='ephemeral'");
    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( { {"name","ephemeral"},{"value","true"} } );
    auto expectedResponse = success({{"name",livequery::ResponseData::AcceptedTypes::STRING},
                                     {"value",livequery::ResponseData::AcceptedTypes::STRING}}, columnData);
    EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
}


TEST_F(TestOSQueryProcessor, QueryShouldBeResitentToIntermitentFailure) // NOLINT
{
    m_osqueryProcess->kill();
    std::atomic<bool> keepRunning = true;
    auto keep_restarting_osquery= std::async(std::launch::async, [this, &keepRunning](){
        for(int i=0; i<10;i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto osq = this->startOsquery();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        keepRunning = false;
    });
    std::string m_testFakeSocketPath(osquerySocket());
    osqueryclient::OsqueryProcessor osqueryProcessor(m_testFakeSocketPath);


    livequery::ResponseData::ColumnData columnData;
    columnData.push_back( { {"name","ephemeral"},{"value","true"} } );
    auto expectedResponse = success({{"name",livequery::ResponseData::AcceptedTypes::STRING},
                                     {"value",livequery::ResponseData::AcceptedTypes::STRING}}, columnData);

    for(int i=0; i<10; i++)
    {
        if (!keepRunning)
        {
            break;
        }
        auto response = osqueryProcessor.query("select name,value from osquery_flags where name=='ephemeral'");
        EXPECT_PRED_FORMAT2(responseIsEquivalent, response, expectedResponse);
    }
}


::testing::AssertionResult TestOSQueryProcessor::responseIsEquivalent(
        const char* m_expr,
        const char* n_expr,
        const livequery::QueryResponse& expectedResponse,
        const livequery::QueryResponse& actualResponse)
{
    std::stringstream s;
    s << m_expr << " and " << n_expr << " failed: ";
    if (expectedResponse.status().errorCode() != actualResponse.status().errorCode())
    {
        return ::testing::AssertionFailure() << s.str() << " Expected response status : " << expectedResponse.status().errorValue()
                                             << ". Actual response status: " << expectedResponse.status().errorValue() << ".";
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