/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "SigHupMonitor.h"

#include <cassert>
#include <csignal>

bool common::SigHupMonitor::triggered()
{
    while (m_pipe.notified())
    {
        m_signalled = true;
    }
    return m_signalled;
}

static int SIGHUP_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void signal_handler(int)
{
    if (SIGHUP_MONITOR_PIPE >= 0)
    {
        auto ret = ::write(SIGHUP_MONITOR_PIPE, "\0", 1);
        /*
         * We are in a signal-context, so are very limited on what we can do.
         * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
         * So we assert for debug builds and do nothing for release builds
         */
        static_cast<void>(ret);
        assert(ret == 1);
    }
}

common::SigHupMonitor::SigHupMonitor()
{
    // Setup signal handler
    struct sigaction action{};
    action.sa_handler = signal_handler;
    action.sa_flags = SA_RESTART;
    int ret = ::sigaction(SIGHUP, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to setup SIGTERM signal handler");
    }
    SIGHUP_MONITOR_PIPE = m_pipe.writeFd();
}

common::SigHupMonitor::~SigHupMonitor()
{
    // clear signal handler
    SIGHUP_MONITOR_PIPE = -1;
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    action.sa_flags = 0;
    int ret = ::sigaction(SIGHUP, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to clear SIGTERM signal handler");
    }
}

std::shared_ptr<common::SigHupMonitor> common::SigHupMonitor::getSigHupMonitor()
{
    static std::shared_ptr<common::SigHupMonitor> monitor(std::make_shared<common::SigHupMonitor>());
    return monitor;
}
