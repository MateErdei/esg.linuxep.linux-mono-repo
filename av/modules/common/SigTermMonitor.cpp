/******************************************************************************************************

Copyright 2020-2022, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "Logger.h"
#include "SigTermMonitor.h"

#include <cassert>
#include <csignal>

namespace common
{
    namespace
    {
        int SIGTERM_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        void signal_handler(int)
        {
            if (SIGTERM_MONITOR_PIPE >= 0)
            {
                [[maybe_unused]] ssize_t ret = ::write(SIGTERM_MONITOR_PIPE, "\0", 1);
                /*
                 * We are in a signal-context, so are very limited on what we can do.
                 * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
                 * So we assert for debug builds and do nothing for release builds
                 */
                assert(ret == 1);
            }
        }
    }

    int SigTermMonitor::monitorFd()
    {
        return m_pipe.readFd();
    }

    bool SigTermMonitor::triggered()
    {
        while (m_pipe.notified())
        {
            m_signalled = true;
        }
        return m_signalled;
    }

    SigTermMonitor::SigTermMonitor()
    {
        // Setup signal handler
        struct sigaction action {};
        action.sa_handler = signal_handler;
        action.sa_flags = SA_RESTART;
        int ret = ::sigaction(SIGTERM, &action, nullptr);
        if (ret != 0)
        {
            LOGERROR("Failed to setup SIGTERM signal handler");
        }
        SIGTERM_MONITOR_PIPE = m_pipe.writeFd();
    }

    SigTermMonitor::~SigTermMonitor()
    {
        // clear signal handler
        SIGTERM_MONITOR_PIPE = -1;
        struct sigaction action {};
        action.sa_handler = SIG_IGN;
        action.sa_flags = 0;
        int ret = ::sigaction(SIGTERM, &action, nullptr);
        if (ret != 0)
        {
            LOGERROR("Failed to clear SIGTERM signal handler");
        }
    }

    std::shared_ptr<SigTermMonitor> SigTermMonitor::getSigTermMonitor()
    {
        static std::shared_ptr<SigTermMonitor> monitor(std::make_shared<SigTermMonitor>());
        return monitor;
    }
} // namespace common