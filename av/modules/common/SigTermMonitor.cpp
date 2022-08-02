// Copyright 2020-2022, Sophos Limited.  All rights reserved.

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
                [[maybe_unused]] auto ret = ::write(SIGTERM_MONITOR_PIPE, "\0", 1);
                /*
                 * We are in a signal-context, so are very limited on what we can do.
                 * http://manpages.ubuntu.com/manpages/bionic/man7/signal-safety.7.html
                 * So we assert for debug builds and do nothing for release builds
                 */
                assert(ret == 1);
            }
        }
    }

    SigTermMonitor::SigTermMonitor()
        : LatchingSignalHandler(SIGTERM)
    {
        // Setup signal handler
        SIGTERM_MONITOR_PIPE = setSignalHandler(signal_handler);
    }

    SigTermMonitor::~SigTermMonitor()
    {
        // clear signal handler
        SIGTERM_MONITOR_PIPE = -1;
        clearSignalHandler();
    }

    std::shared_ptr<SigTermMonitor> SigTermMonitor::getSigTermMonitor()
    {
        static std::shared_ptr<SigTermMonitor> monitor(std::make_shared<SigTermMonitor>());
        return monitor;
    }
} // namespace common