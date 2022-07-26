/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "SigIntMonitor.h"

#include <cassert>
#include <csignal>

bool common::SigIntMonitor::triggered()
{
    while (m_pipe.notified())
    {
        m_signalled = true;
    }
    return m_signalled;
}

static int SIGINT_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void signal_handler(int)
{
    if (SIGINT_MONITOR_PIPE >= 0)
    {
        auto ret = ::write(SIGINT_MONITOR_PIPE, "\0", 1);
        /*
         * We are in a signal-context, so are very limited on what we can do.
         * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
         * So we assert for debug builds and do nothing for release builds
         */
        static_cast<void>(ret);
        assert(ret == 1);
    }
}

common::SigIntMonitor::SigIntMonitor()
{
    // Setup signal handler
    struct sigaction action{};
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    int ret = ::sigaction(SIGINT, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to setup SIGINT signal handler");
    }
    SIGINT_MONITOR_PIPE = m_pipe.writeFd();
}

common::SigIntMonitor::~SigIntMonitor()
{
    // clear signal handler
    SIGINT_MONITOR_PIPE = -1;
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    action.sa_flags = 0;
    int ret = ::sigaction(SIGINT, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to clear SIGINT signal handler");
    }
}

std::shared_ptr<common::SigIntMonitor> common::SigIntMonitor::getSigIntMonitor()
{
    static std::shared_ptr<common::SigIntMonitor> monitor(std::make_shared<common::SigIntMonitor>());
    return monitor;
}