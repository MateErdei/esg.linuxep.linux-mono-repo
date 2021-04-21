/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "sophos_threat_detector/sophosthreatdetectorimpl/SigUSR1Monitor.h"
#include "../../common/LogInitializedTests.h"

#include <gtest/gtest.h>

#include <csignal>

#include <sys/types.h>

/* According to POSIX.1-2001, POSIX.1-2008 */
#include <sys/select.h>

/* According to earlier standards */
#include <common/FDUtils.h>

#include <sys/time.h>
#include <unistd.h>

namespace
{
    class TestSigUSR1Monitor : public LogInitializedTests
    {};

    class FakeReloadable : public sspl::sophosthreatdetectorimpl::IReloadable
    {
    public:
        int m_reloadCount = 0;
        void reload() override
        {
            m_reloadCount++;
        }
    };
}

TEST_F(TestSigUSR1Monitor, testConstruction) // NOLINT
{
    auto reloadable = std::make_shared<FakeReloadable>();
    sspl::sophosthreatdetectorimpl::SigUSR1Monitor monitor(reloadable);
}

TEST_F(TestSigUSR1Monitor, testSignal) // NOLINT
{
    auto reloadable = std::make_shared<FakeReloadable>();
    sspl::sophosthreatdetectorimpl::SigUSR1Monitor monitor(reloadable);
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


TEST_F(TestSigUSR1Monitor, triggerCallsReload) // NOLINT
{
    auto reloadable = std::make_shared<FakeReloadable>();
    sspl::sophosthreatdetectorimpl::SigUSR1Monitor monitor(reloadable);
    monitor.triggered();
    EXPECT_EQ(reloadable->m_reloadCount, 1);
}
