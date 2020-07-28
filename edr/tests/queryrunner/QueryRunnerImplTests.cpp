/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <modules/queryrunner/QueryRunnerImpl.h>
#include <modules/queryrunner/ResponseStatus.h>

#include <gtest/gtest.h>
using namespace ::testing;
    
queryrunner::QueryRunnerStatus statusFromExitResult( int exitCode, const std::string & output)
{
    queryrunner::QueryRunnerStatus st; 
    queryrunner::QueryRunnerImpl::setStatusFromExitResult(st, exitCode, output); 
    return st; 
}

class QueryRunnerImpl : public LogOffInitializedTests{};

TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldNotTryToProcessFurtherOnExitCodeDifferentFrom0) // NOLINT
{
    std::string output{R"({"name":"query", "errorcode":"Success", "duration":10, "rowcount":5})"};
    for(int i=1;i<255;i++)
    {
        auto status = statusFromExitResult( i, output); 
        EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
        EXPECT_EQ(status.name, ""); 
        EXPECT_EQ(status.queryDuration, 0);
        EXPECT_EQ(status.rowCount, 0);
    }
}

TEST_F(QueryRunnerImpl, setStatusFromExitResult_successShouldRetrieveAllInformation) // NOLINT
{
    std::string output{R"({"name":"query", "errorcode":"Success", "duration":10, "rowcount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);
}

TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldTryToFindTheJsonEntry) // NOLINT
{
    std::string output{R"(Extra log that make their way to the stdout
    {"name":"query", "errorcode":"Success", "duration":10, "rowcount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);
}

TEST_F(QueryRunnerImpl, setStatusFromExitResult_successMayIgnoreMissingEntries) // NOLINT
{
    std::string output{R"({"name":"query", "errorcode":"Success", "rowcount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 5);
    
    output = R"({"name":"query", "errorcode":"Success", "duration":10})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 0);
}

TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseToProcessIfErrorCodeNotPresent) // NOLINT
{
    std::string output{R"({"name":"query", "duration":10, "rowcount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}


TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldInterpretCorrectlyTheErrorCode) // NOLINT
{
    std::string output = R"({"name":"query", "errorcode":"Success", "duration":10, "rowcount":5})";
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);

    output = R"({"name":"query", "errorcode":"OsqueryError", "duration":10, "rowcount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::OSQUERYERROR); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorcode":"ExceedLimit", "duration":10, "rowcount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::RESPONSEEXCEEDLIMIT); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorcode":"UnexpectedError", "duration":10, "rowcount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorcode":"ExtensionExited", "duration":10, "rowcount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    // will refuse to process further if errorCode is Invalid
    output = R"({"name":"query", "errorcode":"NotAValiderrorcode", "duration":10, "rowcount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);    

}


TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseIfNoJsonIsPresent) // NOLINT
{
    std::string output{R"(not a json)"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}

TEST_F(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseIfJsonIsInvalid) // NOLINT
{
    std::string output{R"({"name":"query", "duration":10, "rowcount":5 not closed)"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}

