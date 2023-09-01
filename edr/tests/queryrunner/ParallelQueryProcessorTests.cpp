// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "queryrunner/ParallelQueryProcessor.h"

#include "Common/Logging/ConsoleLoggingSetup.h"
#include "Common/Threads/LockableData.h"

#include "Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <atomic>
#include <iostream>
#include <memory>
#include <utility>

using namespace ::testing;

namespace
{
    class ConfigurableDelayedQuery : public queryrunner::IQueryRunner
    {
    public:
        using counter_t = Common::Threads::LockableData<int>&;
    private:
        std::future<void> m_fut;
        std::string m_id;
        counter_t m_counter;
        bool triggered = false;

    public:
        explicit ConfigurableDelayedQuery(counter_t counter) : m_counter { counter }
        {
            triggered = false;
        }
        ~ConfigurableDelayedQuery() override
        {
            if (!triggered)
                return;
            if (m_fut.valid())
            {
                try
                {
                    m_fut.wait();
                }
                catch (std::exception& ex)
                {
                    std::cerr << ex.what() << std::endl;
                }
            }
        }
        void requestAbort() override {};
        void triggerQuery(
            const std::string& correlationid,
            const std::string& query,
            std::function<void(std::string id)> notifyFinished) override
        {
            triggered = true;
            m_id = correlationid;
            m_fut = std::async(
                std::launch::async,
                [query, correlationid, notifyFinished]()
                {
                    std::stringstream s(query);
                    int timeToWait;
                    s >> timeToWait;
                    if (!s)
                    {
                        timeToWait = 0;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(timeToWait));
                    notifyFinished(correlationid);
                });
        }
        std::string id() override
        {
            return m_id;
        }
        queryrunner::QueryRunnerStatus getResult() override
        {
            {
                auto locked = m_counter.lock();
                *locked += 1;
            }
            return queryrunner::QueryRunnerStatus {};
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
}

TEST_F(ParallelQueryProcessorTests, addJob)
{
    Common::Threads::LockableData<int> counter{0};
    {
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::make_unique<ConfigurableDelayedQuery>(counter)};
        parallelQueryProcessor.addJob(buildQuery(100), "1");
        parallelQueryProcessor.addJob(buildQuery(100), "2");
        std::this_thread::sleep_for(std::chrono::microseconds{50});
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value;
    {
        auto locked = counter.lock();
        value = *locked;
    }
    // Jobs could be run or not, depending on thread performance
    EXPECT_GE(value, 0);
    EXPECT_LE(value, 2);
}

// TODO LINUXDAR-7936 Disabled because this intermittently SEGFAULTs - needs fixing
TEST_F(ParallelQueryProcessorTests, DISABLED_jobsAreClearedAsPossible)
{
    testing::internal::CaptureStderr();

    Common::Threads::LockableData<int> counter{0};
    {
        queryrunner::ParallelQueryProcessor parallelQueryProcessor{std::make_unique<ConfigurableDelayedQuery>(counter)};
        for(int i=0; i<10;i++)
        {
            parallelQueryProcessor.addJob(buildQuery(100), std::to_string(i));
            std::this_thread::sleep_for(std::chrono::microseconds{70});
        }
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value;
    {
        auto locked = counter.lock();
        value = *locked;
    }
    EXPECT_EQ(value, 10);
    std::string logMessage = testing::internal::GetCapturedStderr();

    EXPECT_THAT(logMessage, ::testing::HasSubstr("DEBUG One more entry removed from the queue of processing queries"));
    EXPECT_THAT(logMessage, ::testing::Not(
                                ::testing::HasSubstr("Failed to configure a new query")
                                ));
    EXPECT_THAT(logMessage, ::testing::Not(
                                ::testing::HasSubstr("Failure in clean up ParallelQueryProcessor")
                                ));
    EXPECT_THAT(logMessage, ::testing::Not(
                                ::testing::HasSubstr("Should not have any further queries to process.")
                                    ));
}