/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/livequery/ParallelQueryProcessor.h>
#include <modules/livequery/QueryResponse.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

using namespace ::testing;

class ConfigurableDelayedQuery : public livequery::IQueryProcessor
{
public:
    ConfigurableDelayedQuery()
    {

    }
    livequery::QueryResponse query(const std::string& query) override
    {
        std::stringstream s(query);
        int timeToWait;
        s>>timeToWait;
        if (!s)
        {
            return livequery::QueryResponse::emptyResponse();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
        return livequery::QueryResponse::emptyResponse();
    };

    std::unique_ptr<livequery::IQueryProcessor> clone() override {
        return std::unique_ptr<livequery::IQueryProcessor>(new ConfigurableDelayedQuery{});
    }
};

class DummyResponse: public livequery::IResponseDispatcher
{
public:
    DummyResponse(std::shared_ptr<std::atomic<int>> counter) : m_counter{counter}{}
    void sendResponse(const std::string& , const livequery::QueryResponse& ) override {
        m_counter->fetch_add(1);
    };
    std::unique_ptr<livequery::IResponseDispatcher> clone() override {
        return std::unique_ptr<livequery::IResponseDispatcher>(new DummyResponse{m_counter});
    };
    std::shared_ptr<std::atomic<int>> m_counter;
};

std::string buildQuery(int timeToSleep)
{
    return R"({"type":"sophos.mgt.action.RunLiveQuery", "name":"test", "query": ")" + std::to_string(timeToSleep) + "\"}";
}

class ParallelQueryProcessorTests : public ::testing::Test
{

};

TEST_F(ParallelQueryProcessorTests, addJob) // NOLINT
{
    auto counter = std::make_shared<std::atomic<int>>(0);
    {
        livequery::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<livequery::IQueryProcessor>(new ConfigurableDelayedQuery{}),
                                                                 std::unique_ptr<livequery::IResponseDispatcher>(new DummyResponse{counter})  };
        parallelQueryProcessor.addJob(buildQuery(1), "1");
        parallelQueryProcessor.addJob(buildQuery(1), "2");
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value = *counter;
    EXPECT_EQ(value, 2);
}

TEST_F(ParallelQueryProcessorTests, jobsAreClearedAsPossible) // NOLINT
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    auto counter = std::make_shared<std::atomic<int>>(0);
    {
        livequery::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<livequery::IQueryProcessor>(new ConfigurableDelayedQuery{}),
                                                                 std::unique_ptr<livequery::IResponseDispatcher>(new DummyResponse{counter})  };
        for(int i=0; i<10;i++)
        {
            parallelQueryProcessor.addJob(buildQuery(1), std::to_string(i));
            std::this_thread::sleep_for(std::chrono::microseconds{200});
        }
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value = *counter;
    EXPECT_EQ(value, 10);
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("DEBUG One more entry removed from the queue of processing queries"));
}