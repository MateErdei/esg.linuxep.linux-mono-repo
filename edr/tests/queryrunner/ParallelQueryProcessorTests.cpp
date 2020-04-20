/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <modules/queryrunner/ParallelQueryProcessor.h>
#include <modules/livequery/QueryResponse.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <gtest/gtest.h>
#include <tests/googletest/googlemock/include/gmock/gmock-matchers.h>

using namespace ::testing;
    

class ConfigurableDelayedQuery : public queryrunner::IQueryRunner
{
    std::future<void> m_fut; 
    std::string m_id; 
    
public:
    ConfigurableDelayedQuery()
    {

    }
    ~ConfigurableDelayedQuery()
    {
        if (m_fut.valid())
        {
            m_fut.wait(); 
        }
    }
    void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) override
    {
        m_id = correlationid; 
        m_fut = std::async(std::launch::async, [query, correlationid, notifyFinished](){
        std::stringstream s(query);
        int timeToWait;
        s>>timeToWait;
        if (!s)
        {
            timeToWait = 0; 
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
        notifyFinished(correlationid); 
        }); 
    }
    std::string id() override{
        return m_id; 
    }
    virtual queryrunner::QueryRunnerStatus getResult() override
    {
        return queryrunner::QueryRunnerStatus{};
    }
    std::unique_ptr<IQueryRunner> clone() override{
        return std::unique_ptr<IQueryRunner>(new ConfigurableDelayedQuery{});
    };  
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
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<queryrunner::IQueryRunner>(new ConfigurableDelayedQuery{})};
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
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<queryrunner::IQueryRunner>(new ConfigurableDelayedQuery{})};
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