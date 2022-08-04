// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "modules/common/signals/SigUSR1Monitor.h"
#include "tests/common/LogInitializedTests.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <csignal>

#include <sys/types.h>

/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>

/* According to earlier standards */
#include <common/FDUtils.h>

#include <sys/time.h>
#include <unistd.h>

using namespace common::signals;

namespace
{
    class TestSigUSR1Monitor: public LogOffInitializedTests
    {
    };

    class MockReloadable : public IReloadable
    {
    public:
        MOCK_METHOD(void, update, (), (override));
        MOCK_METHOD(void, reload, (), (override));
    };
}

namespace
{
    class FakeReloadable : public IReloadable
    {
    public:
        int m_reloadCount = 0;
        int m_updateCount = 0;
        void update() override
        {
            m_updateCount++;
        }
        void reload() override
        {
            m_reloadCount++;
        }
    };
}

TEST_F(TestSigUSR1Monitor, testConstruction)
{
    auto reloadable = std::make_shared<FakeReloadable>();
    SigUSR1Monitor monitor(reloadable);
}

TEST_F(TestSigUSR1Monitor, testSignal)
{
    auto reloadable = std::make_shared<FakeReloadable>();
    SigUSR1Monitor monitor(reloadable);
    ::kill(::getpid(), SIGUSR1);
    // notify pipe not exposed so need to check the fd
    int readFd = monitor.monitorFd();

    fd_set readfds{};
    FD_ZERO(&readfds);
    FD_SET(readFd, &readfds); // NOLINT
    struct timeval zero_time{};
    zero_time.tv_sec = 0;
    zero_time.tv_usec = 0;

    int ret = ::select(readFd+1, &readfds, nullptr, nullptr, &zero_time);

    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(FDUtils::fd_isset(readFd, &readfds)); // NOLINT
}

TEST_F(TestSigUSR1Monitor, triggerCallsUpdate)
{
    auto reloadable = std::make_shared<FakeReloadable>();
    SigUSR1Monitor monitor(reloadable);
    monitor.triggered();
    EXPECT_EQ(reloadable->m_updateCount, 1);
}
