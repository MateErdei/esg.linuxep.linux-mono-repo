/***********************************************************************************************

Copyright 2021-2021 Sophos Limited. All rights reserved.

***********************************************************************************************/

#include <Common/Helpers/LogInitializedTests.h>
#include <modules/osqueryextensions/BatchTimer.h>

#include <chrono>
#include <memory>

#include <gtest/gtest.h>

class TestBatchTimer : public LogOffInitializedTests
{
protected:
    void SetUp() override
    {
        m_timer.reset(new BatchTimer);
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

TEST_F(TestBatchTimer, CallbackRunOnTimeout) // NOLINT
{
    m_timer->Configure([this]{ timerCallback(); }, std::chrono::milliseconds(MAX_TIME_MILLISECONDS));

    auto startTime = std::chrono::steady_clock::now();
    m_timer->Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(3 * MAX_TIME_MILLISECONDS));

    ASSERT_GT(m_callbackTime, startTime);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_callbackTime - startTime);
    ASSERT_GE(duration.count(), MAX_TIME_MILLISECONDS);
}

TEST_F(TestBatchTimer, CallbackNotRunIfCancelled) // NOLINT
{
    m_timer->Configure([this]{ timerCallback(); }, std::chrono::milliseconds(MAX_TIME_MILLISECONDS));

    m_timer->Start();
    m_timer->Cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(3 * MAX_TIME_MILLISECONDS));

    EXPECT_EQ(0, m_callbackTime.time_since_epoch().count());
}

TEST_F(TestBatchTimer, CallbackNotRunIfDestroyed) // NOLINT
{
    m_timer->Configure([this]{ timerCallback(); }, std::chrono::milliseconds(MAX_TIME_MILLISECONDS));

    m_timer->Start();

    m_timer.reset();

    std::this_thread::sleep_for(std::chrono::milliseconds(3 * MAX_TIME_MILLISECONDS));

    EXPECT_EQ(0, m_callbackTime.time_since_epoch().count());
}

TEST_F(TestBatchTimer, UnconfiguredCallbackThrows) // NOLINT
{
    EXPECT_THROW(m_timer->Start(), std::logic_error);
}

TEST_F(TestBatchTimer, CallbackExceptionMessage) // NOLINT
{
    m_timer->Configure([this]{ throw std::runtime_error("test exception"); }, std::chrono::milliseconds(MAX_TIME_MILLISECONDS));

    m_timer->Start();

    std::this_thread::sleep_for(std::chrono::milliseconds(3 * MAX_TIME_MILLISECONDS));

    EXPECT_STREQ("test exception", m_timer->getCallbackError().c_str());
}
