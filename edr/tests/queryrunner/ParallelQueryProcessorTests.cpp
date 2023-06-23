// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include <modules/queryrunner/ParallelQueryProcessor.h>
#include <Common/Logging/ConsoleLoggingSetup.h>
#include <Common/Helpers/LogInitializedTests.h>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <atomic>
#include <memory>
#include <utility>
using namespace ::testing;
    

class ConfigurableDelayedQuery : public queryrunner::IQueryRunner
{
    std::future<void> m_fut; 
    std::string m_id; 
    std::shared_ptr<std::atomic<int>> m_counter;
    bool triggered = false; 
public:
    explicit ConfigurableDelayedQuery(std::shared_ptr<std::atomic<int>> counter)
        : m_counter{std::move(counter)}
    {
        triggered = false; 
    }
    ~ConfigurableDelayedQuery() override
    {
        if (!triggered) return; 
        if (m_fut.valid())
        {
            try{
                m_fut.wait(); 
            }catch(std::exception & ex)
            {
                std::cerr << ex.what() << std::endl; 
            }            
        }
    }
    void requestAbort() override{}; 
    void triggerQuery(const std::string& correlationid, const std::string& query, std::function<void(std::string id)> notifyFinished) override
    {
        triggered = true; 
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
    std::string id() override
    {
        return m_id; 
    }
    queryrunner::QueryRunnerStatus getResult() override
    {
        *m_counter+=1;
        return queryrunner::QueryRunnerStatus{};
    }
    std::unique_ptr<IQueryRunner> clone() override
    {
        return std::make_unique<ConfigurableDelayedQuery>(m_counter);
    }
};

std::string buildQuery(int timeToSleep)
{
    return R"({"type":"sophos.mgt.action.RunLiveQuery", "name":"test", "query": ")" + std::to_string(timeToSleep) + "\"}";
}

class ParallelQueryProcessorTests : public LogInitializedTests{};

TEST_F(ParallelQueryProcessorTests, addJob)
{
    auto counter = std::make_shared<std::atomic<int>>(0);
    {
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::make_unique<ConfigurableDelayedQuery>(counter)};
        parallelQueryProcessor.addJob(buildQuery(1), "1");
        parallelQueryProcessor.addJob(buildQuery(1), "2");
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value = *counter;
    // Jobs could be run or not, depending on thread performance
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 2);
}

TEST_F(ParallelQueryProcessorTests, jobsAreClearedAsPossible)
{
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    auto counter = std::make_shared<std::atomic<int>>(0);
    {
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::unique_ptr<queryrunner::IQueryRunner>(new ConfigurableDelayedQuery{counter})};
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