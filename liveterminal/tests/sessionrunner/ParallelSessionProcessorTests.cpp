// Copyright 2023 Sophos Limited. All rights reserved.

#include "sessionrunner/ParallelSessionProcessor.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/Logging/ConsoleLoggingSetup.h"

#ifdef SPL_BAZEL
#include "base/tests/Common/Helpers/FileSystemReplaceAndRestore.h"
#include "base/tests/Common/Helpers/LogInitializedTests.h"
#include "base/tests/Common/Helpers/MockFileSystem.h"
#else
#include "Common/Helpers/FileSystemReplaceAndRestore.h"
#include "Common/Helpers/LogInitializedTests.h"
#include "Common/Helpers/MockFileSystem.h"
#endif

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include <atomic>
#include <memory>

using namespace ::testing;

class ConfigurableDelayedSession : public sessionrunner::ISessionRunner
{
    std::future<void> m_fut;
    std::string m_id;
    std::shared_ptr<std::atomic<int>> m_counter;
    bool triggered = false;

public:
    explicit ConfigurableDelayedSession(std::shared_ptr<std::atomic<int>> counter) : m_counter{ counter }
    {
        triggered = false;
    }
    ~ConfigurableDelayedSession()
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
    void requestAbort() override{};
    void triggerSession(const std::string& id, std::function<void(std::string id)> notifyFinished) override
    {
        *m_counter += 1;
        triggered = true;
        m_id = id;
        m_fut = std::async(
            std::launch::async,
            [id, notifyFinished]()
            {
                std::stringstream s(id);
                int timeToWait;
                s >> timeToWait;
                if (!s)
                {
                    timeToWait = 0;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(timeToWait));
                notifyFinished(id);
            });
    }
    std::string id() override { return m_id; }
    std::unique_ptr<ISessionRunner> clone() override
    {
        return std::unique_ptr<ISessionRunner>(new ConfigurableDelayedSession{ m_counter });
    };
};

class ParallelSessionProcessorTests : public LogOffInitializedTests
{
};

TEST_F(ParallelSessionProcessorTests, tenJobsAreAddedAndClearedSuccessfully)
{
    auto filesystemMock = new StrictMock<MockFileSystem>();
    EXPECT_CALL(*filesystemMock, removeFile(_)).Times(10);
    EXPECT_CALL(*filesystemMock, isFile(_)).Times(10).WillRepeatedly(Return(true));
    Tests::replaceFileSystem(std::unique_ptr<Common::FileSystem::IFileSystem>(filesystemMock));
    Common::Logging::ConsoleLoggingSetup consoleLoggingSetup;
    testing::internal::CaptureStderr();

    auto counter = std::make_shared<std::atomic<int>>(0);
    {
        sessionrunner::ParallelSessionProcessor parallelQueryProcessor{
            std::unique_ptr<sessionrunner::ISessionRunner>(new ConfigurableDelayedSession{ counter }), "/somepath"
        };
        for (int i = 0; i < 10; i++)
        {
            parallelQueryProcessor.addJob(std::to_string(i));
            std::this_thread::sleep_for(std::chrono::microseconds{ 200 });
        }
    }
    // whenever parallel is destroyed, all the jobs will have been finished.
    int value = *counter;
    EXPECT_EQ(value, 10);
}