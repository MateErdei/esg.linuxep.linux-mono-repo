// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "MockQueryProcessor.h"
#include "MockResponseDispatcher.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"
#include "livequery/Logger.h"


#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace ::testing;

class TestQueryProcessor : public ::testing::Test
{
public:
    std::string validJsonRequest()
    {
        return R"({
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": "SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2"
})";
    }

    std::string requestWithNonStringQuery()
    {
        return R"({
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": 3908
})";
    }

    enum class FieldMiss{TYPE,NAME,QUERY};
    std::string missingField(FieldMiss fieldMiss)
    {
        switch (fieldMiss)
        {
            case FieldMiss::TYPE:
                return R"({
    "name": "Top 2 Processes",
    "query": "SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2"
})";
            case FieldMiss::NAME:
                return R"({
    "type": "sophos.mgt.action.RunLiveQuery",
    "query": "SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2"
})";
            case FieldMiss::QUERY:
                return R"({
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes"
})";
        }
        return R"({
    "type": "sophos.mgt.action.RunLiveQuery",
    "name": "Top 2 Processes",
    "query": "SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2"
})";

    }

    std::string notJson()
    {
        return R"(<xml Doc /> )";
    }
};

TEST_F(TestQueryProcessor, ValidJsonRequestShouldProcessedByIQueryProcessorAndResponseForwardedToDispatcher)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();

    LOGDEBUG("Produce this logging");
    MockQueryProcessor mockQueryProcessor;
    MockResponseDispatcher mockResponseDispatcher;

    EXPECT_CALL(mockQueryProcessor, query("SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2")).WillOnce(testing::Return(livequery::QueryResponse::emptyResponse()));
    EXPECT_CALL(mockResponseDispatcher, sendResponse("correlation-id",_)).Times(1);
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, validJsonRequest(), "correlation-id");

    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("Top 2 Processes"));
}


TEST_F(TestQueryProcessor, RequestWithInvalidJsonShouldBeRejected)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();
    MockQueryProcessor mockQueryProcessor;
    MockResponseDispatcher mockResponseDispatcher;

    EXPECT_CALL(mockQueryProcessor, query(_)).Times(0);
    EXPECT_CALL(mockResponseDispatcher, sendResponse(_,_)).Times(0);
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, notJson(), "correlation-id");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Received an invalid request, failed to parse the json input."));
}

TEST_F(TestQueryProcessor, RequestWithMissingFieldsFromJsonShouldBeRejected)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();
    MockQueryProcessor mockQueryProcessor;
    MockResponseDispatcher mockResponseDispatcher;

    EXPECT_CALL(mockQueryProcessor, query(_)).Times(0);
    EXPECT_CALL(mockResponseDispatcher, sendResponse(_,_)).Times(0);
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, missingField(TestQueryProcessor::FieldMiss::TYPE), "correlation-id");
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, missingField(TestQueryProcessor::FieldMiss::NAME), "correlation-id");
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, missingField(TestQueryProcessor::FieldMiss::QUERY), "correlation-id");


    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Invalid request, required json values are:"));
}


TEST_F(TestQueryProcessor, JsonWithNonString)
{
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    testing::internal::CaptureStderr();
    MockQueryProcessor mockQueryProcessor;
    MockResponseDispatcher mockResponseDispatcher;

    EXPECT_CALL(mockQueryProcessor, query(_)).Times(0);
    EXPECT_CALL(mockResponseDispatcher, sendResponse(_,_)).Times(0);
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, requestWithNonStringQuery(), "correlation-id");


    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Invalid request, required json value 'query' must be a string"));
}




TEST_F(TestQueryProcessor, QueryProcessorFailureShouldNotCrash)
{
    MockQueryProcessor mockQueryProcessor;
    MockResponseDispatcher mockResponseDispatcher;
    testing::internal::CaptureStderr();
    Common::Logging::ConsoleLoggingSetup consoleLogger;
    EXPECT_CALL(mockQueryProcessor, query("SELECT sophosPID, pathname, start_time FROM sophos_process_journal limit 2")).WillOnce(testing::Invoke(
            [](const std::string&){throw std::bad_alloc{}; return livequery::QueryResponse::emptyResponse(); }));
    EXPECT_CALL(mockResponseDispatcher, sendResponse("correlation-id",_)).Times(0);
    livequery::processQuery(mockQueryProcessor, mockResponseDispatcher, validJsonRequest(), "correlation-id");

    std::string logMessage = testing::internal::GetCapturedStderr();
    EXPECT_THAT(logMessage, ::testing::HasSubstr("Error while executing query"));
}


