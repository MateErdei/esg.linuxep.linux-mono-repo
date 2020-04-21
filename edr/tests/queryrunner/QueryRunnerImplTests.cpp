/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/queryrunner/QueryRunnerImpl.h>
#include <modules/livequery/ResponseStatus.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>
using namespace ::testing;
    
queryrunner::QueryRunnerStatus statusFromExitResult( int exitCode, const std::string & output)
{
    queryrunner::QueryRunnerStatus st; 
    queryrunner::QueryRunnerImpl::setStatusFromExitResult(st, exitCode, output); 
    return st; 
}

TEST(QueryRunnerImpl, setStatusFromExitResult_shouldNotTryToProcessFurtherOnExitCodeDifferentFrom0) // NOLINT
{
    std::string output{R"({"name":"query", "errorCode":"Success", "duration":10, "rowCount":5})"};
    for(int i=1;i<255;i++)
    {
        auto status = statusFromExitResult( i, output); 
        EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
        EXPECT_EQ(status.name, ""); 
        EXPECT_EQ(status.queryDuration, 0);
        EXPECT_EQ(status.rowCount, 0);
    }
}

TEST(QueryRunnerImpl, setStatusFromExitResult_successShouldRetrieveAllInformation) // NOLINT
{
    std::string output{R"({"name":"query", "errorCode":"Success", "duration":10, "rowCount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);
}

TEST(QueryRunnerImpl, setStatusFromExitResult_shouldTryToFindTheJsonEntry) // NOLINT
{
    std::string output{R"(Extra log that make their way to the stdout
    {"name":"query", "errorCode":"Success", "duration":10, "rowCount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);
}

TEST(QueryRunnerImpl, setStatusFromExitResult_successMayIgnoreMissingEntries) // NOLINT
{
    std::string output{R"({"name":"query", "errorCode":"Success", "rowCount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 5);
    
    output = R"({"name":"query", "errorCode":"Success", "duration":10})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 0);
}

TEST(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseToProcessIfErrorCodeNotPresent) // NOLINT
{
    std::string output{R"({"name":"query", "duration":10, "rowCount":5})"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}


TEST(QueryRunnerImpl, setStatusFromExitResult_shouldInterpretCorrectlyTheErrorCode) // NOLINT
{
    std::string output = R"({"name":"query", "errorCode":"Success", "duration":10, "rowCount":5})";
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::SUCCESS); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);

    output = R"({"name":"query", "errorCode":"OsqueryError", "duration":10, "rowCount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::OSQUERYERROR); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorCode":"ExceedLimit", "duration":10, "rowCount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::RESPONSEEXCEEDLIMIT); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorCode":"UnexpectedError", "duration":10, "rowCount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    output = R"({"name":"query", "errorCode":"ExtensionExited", "duration":10, "rowCount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING); 
    EXPECT_EQ(status.name, "query"); 
    EXPECT_EQ(status.queryDuration, 10);
    EXPECT_EQ(status.rowCount, 5);    

    // will refuse to process further if errorCode is Invalid
    output = R"({"name":"query", "errorCode":"NotAValidErrorCode", "duration":10, "rowCount":5})";
    status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);    

}


TEST(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseIfNoJsonIsPresent) // NOLINT
{
    std::string output{R"(not a json)"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}

TEST(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseIfJsonIsInvalid) // NOLINT
{
    std::string output{R"({"name":"query", "duration":10, "rowCount":5 not closed)"};
    auto status = statusFromExitResult( 0, output); 
    EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
    EXPECT_EQ(status.name, ""); 
    EXPECT_EQ(status.queryDuration, 0);
    EXPECT_EQ(status.rowCount, 0);
}

// TEST(QueryRunnerImpl, setStatusFromExitResult_shouldRefuseToProcessIfErrorCodeNotPresent) // NOLINT
// {
//     std::string output{R"({"name":"query", "errorCode":"Success", "duration":10, "rowCount":5})"};
//     auto status = statusFromExitResult( 0, output); 
//     EXPECT_EQ(status.errorCode, livequery::ErrorCode::UNEXPECTEDERROR); 
//     EXPECT_EQ(status.name, ""); 
//     EXPECT_EQ(status.queryDuration, 0);
//     EXPECT_EQ(status.rowCount, 0);
// }

// TEST_F(ParallelQueryProcessorTests, jobsAreClearedAsPossible) // NOLINT
// {
//     Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
//     testing::internal::CaptureStderr();

//     auto counter = std::make_shared<std::atomic<int>>(0);
//     {
//         queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<queryrunner::IQueryRunner>(new ConfigurableDelayedQuery{counter})};
//         for(int i=0; i<10;i++)
//         {
//             parallelQueryProcessor.addJob(buildQuery(1), std::to_string(i));
//             std::this_thread::sleep_for(std::chrono::microseconds{200});
//         }
//     }
//     // whenever parallel is destroyed, all the jobs will have been finished.
//     int value = *counter;
//     EXPECT_EQ(value, 10);
//     std::string logMessage = testing::internal::GetCapturedStderr();

//     EXPECT_THAT(logMessage, ::testing::HasSubstr("DEBUG One more entry removed from the queue of processing queries"));
// }