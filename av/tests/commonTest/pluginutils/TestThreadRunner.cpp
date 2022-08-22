// Copyright 2022, Sophos Limited.  All rights reserved.

#include "TestAbstractThreadImpl.h"

#include "common/ThreadRunner.h"
#include "tests/common/MemoryAppender.h"

#include <gtest/gtest.h>


using namespace ::testing;

namespace
{
    class TestThreadRunner : public MemoryAppenderUsingTests
    {
    public:

        TestThreadRunner()
            : MemoryAppenderUsingTests("Common")
        {}

    };

    class TestClassUsingThreadRunner {
    public:
        explicit TestClassUsingThreadRunner(std::shared_ptr<common::AbstractThreadPluginInterface> absThread, std::string name) :
            m_testThreadRunner(std::move(absThread), std::move(name), true)
        {

        }
    public:
        inline void stopThread() { m_testThreadRunner.requestStopIfNotStopped(); }

    private:
        common::ThreadRunner m_testThreadRunner;
    };
}

TEST_F(TestThreadRunner, ThreadRunnerStarts) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto testThread = std::make_shared<TestAbstractThreadImpl>();
    TestClassUsingThreadRunner(testThread, "ThreadRunnerStarts");

    EXPECT_TRUE(appenderContains("Starting ThreadRunnerStarts"));
}

TEST_F(TestThreadRunner, ThreadRunnerStops) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto testThread = std::make_shared<TestAbstractThreadImpl>();
    auto testTRUser = std::make_unique<TestClassUsingThreadRunner>(testThread, "ThreadRunnerStops");
    testTRUser.reset();

    EXPECT_TRUE(appenderContains("Stopping ThreadRunnerStops"));
    EXPECT_TRUE(appenderContains("Request Stop in TestAbstractThreadImpl"));
    EXPECT_TRUE(appenderContains("Joining ThreadRunnerStops"));
}

TEST_F(TestThreadRunner, ThreadRunnerStopsThenJoins)
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto testThread = std::make_shared<TestAbstractThreadImpl>();
    auto testTRUser = std::make_unique<TestClassUsingThreadRunner>(testThread, "ThreadRunnerStopsThenJoins");

    testTRUser->stopThread();
    EXPECT_TRUE(appenderContains("Stopping ThreadRunnerStopsThenJoins"));
    EXPECT_TRUE(appenderContains("Request Stop in TestAbstractThreadImpl"));
    EXPECT_TRUE(appenderContains("Joining ThreadRunnerStopsThenJoins"));

    testTRUser.reset();
    EXPECT_TRUE(appenderContains("Stopping ThreadRunnerStopsThenJoins"));
    EXPECT_TRUE(appenderContains("Request Stop in TestAbstractThreadImpl"));
    EXPECT_TRUE(appenderContains("Joining ThreadRunnerStopsThenJoins"));
}

TEST_F(TestThreadRunner, ThreadRunnerStopsAndStarts) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    auto testThread = std::make_shared<TestAbstractThreadImpl>();
    common::ThreadRunner testThreadRunner(testThread, "ThreadRunnerStopsAndStarts", true);

    EXPECT_TRUE(testThreadRunner.isStarted());
    EXPECT_TRUE(appenderContains("Starting ThreadRunnerStopsAndStarts"));

    testThreadRunner.requestStopIfNotStopped();

    EXPECT_FALSE(testThreadRunner.isStarted());
    EXPECT_TRUE(appenderContains("Stopping ThreadRunnerStopsAndStarts"));
    EXPECT_TRUE(appenderContains("Request Stop in TestAbstractThreadImpl"));
    EXPECT_TRUE(appenderContains("Joining ThreadRunnerStopsAndStarts"));

    testThreadRunner.startIfNotStarted();

    EXPECT_TRUE(testThreadRunner.isStarted());
    EXPECT_TRUE(appenderContains("Starting ThreadRunnerStopsAndStarts"));
}

TEST_F(TestThreadRunner, ThreadRunnerHandlesStartWhenThreadIsNotValid) // NOLINT
{
    UsingMemoryAppender memoryAppenderHolder(*this);

    std::shared_ptr<TestAbstractThreadImpl> nullThread = nullptr;
    common::ThreadRunner testThreadRunner(nullThread, "invalidThreadOnStart", true);

    EXPECT_FALSE(testThreadRunner.isStarted());
    EXPECT_TRUE(appenderContains("Failed to start thread for invalidThreadOnStart"));
}

