// Copyright 2021-2023 Sophos Limited. All rights reserved.

#include "modules/osqueryextensions/BatchTimer.h"

#include "Common/Exceptions/Print.h"

#include <chrono>
#include <memory>
#include <thread>

#include "Common/Helpers/LogInitializedTests.h"

#include <gtest/gtest.h>

namespace
{
    class TestBatchTimer : public LogOffInitializedTests
    {
    protected:
        void SetUp() override
        {
            m_timer = std::make_unique<BatchTimer>();
        }

        void TearDown() override
        {
        }

        void timerCallback()
        {
            m_callbackTime = std::chrono::steady_clock::now();
        }

        static constexpr uint32_t MAX_TIME_MILLISECONDS = 5;

        std::unique_ptr<BatchTimer> m_timer;
        std::chrono::time_point<std::chrono::steady_clock> m_callbackTime;
    };

    class TestBatchTimerWithLogs : public LogInitializedTests
    {
    protected:
        void SetUp() override
        {
            m_timer = std::make_unique<BatchTimer>();
        }

        void TearDown() override
        {
        }

        void timerCallback()
        {
            m_callbackTime = std::chrono::steady_clock::now();
        }

        void resetCallbackTime()
        {
            m_callbackTime = decltype(m_callbackTime){};
        }

        static constexpr uint32_t MAX_TIME_MILLISECONDS = 5;

        std::unique_ptr<BatchTimer> m_timer;
        std::chrono::time_point<std::chrono::steady_clock> m_callbackTime;
    };
}

TEST_F(TestBatchTimer, CallbackRunOnTimeout)
{
    for (auto i=1; i<20; ++i)
    {
        auto delay = 3 * i;

        m_timer->Configure([this] { timerCallback(); }, std::chrono::milliseconds(delay));

        auto startTime = std::chrono::steady_clock::now();
        m_timer->Start();

        std::this_thread::sleep_for(std::chrono::milliseconds(3 * delay));

        if (m_callbackTime.time_since_epoch().count() == 0)
        {
            // callback never called - try again with longer timeout
            continue;
        }
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_callbackTime - startTime);
        if (duration.count() >= delay)
        {
            // success
            return;
        }
    }
    ASSERT_NE(m_callbackTime.time_since_epoch().count(), 0);
    FAIL() << "Timeout duration always less than expected";
}

TEST_F(TestBatchTimerWithLogs, CallbackNotRunIfCancelled)
{
    for (auto i=1; i<40; ++i)
    {
        auto delay = 3 * i;

        m_timer->Configure([this]{ timerCallback(); }, std::chrono::milliseconds(delay));

        m_timer->Start();
        m_timer->Cancel();

        std::this_thread::sleep_for(std::chrono::milliseconds(3 * delay));

        if (m_callbackTime.time_since_epoch().count() == 0)
        {
            // Success - callback not called
            return;
        }
        // failure - retry with longer interval
        m_timer = std::make_unique<BatchTimer>();
        resetCallbackTime();
    }

    EXPECT_EQ(0, m_callbackTime.time_since_epoch().count());
}

TEST_F(TestBatchTimerWithLogs, CallbackNotRunIfDestroyed)
{

    for (auto i=1; i<50; ++i)
    {
        PRINT("Loop with i=" << i);
        auto delay = 3 * i;

        m_timer->Configure([this]{ timerCallback(); }, std::chrono::milliseconds(delay));
        m_timer->Start();
        m_timer.reset();
        std::this_thread::sleep_for(std::chrono::milliseconds(3 * delay));

        // Callback shouldn't be called, since we should wait delay ms before calling the callback
        if (m_callbackTime.time_since_epoch().count() == 0)
        {
            return;
        }
        m_timer = std::make_unique<BatchTimer>();
        resetCallbackTime();
    }
    EXPECT_EQ(0, m_callbackTime.time_since_epoch().count());
}

TEST_F(TestBatchTimer, UnconfiguredCallbackThrows)
{
    EXPECT_THROW(m_timer->Start(), std::logic_error);
}

TEST_F(TestBatchTimer, CallbackExceptionMessage)
{
    m_timer->Configure([this]{ throw std::runtime_error("test exception"); }, std::chrono::milliseconds(MAX_TIME_MILLISECONDS));
    
    m_timer->Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(MAX_TIME_MILLISECONDS));

    // Confirm that the exception is thrown at some point over the next 50 milliseconds
    int count = 0;
    while (count++ < 50 && m_timer->getCallbackError().compare("test exception") != 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_STREQ("test exception", m_timer->getCallbackError().c_str());
}
