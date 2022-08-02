// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "Logger.h"
#include "SigIntMonitor.h"

#include <cassert>
#include <csignal>

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
    : LatchingSignalHandler(SIGINT)
{
    // Setup signal handler
    SIGINT_MONITOR_PIPE = setSignalHandler(SIGINT, signal_handler);
}

common::SigIntMonitor::~SigIntMonitor()
{
    // clear signal handler
    SIGINT_MONITOR_PIPE = -1;
    clearSignalHandler(SIGINT);
}

std::shared_ptr<common::SigIntMonitor> common::SigIntMonitor::getSigIntMonitor()
{
    static std::shared_ptr<common::SigIntMonitor> monitor(std::make_shared<common::SigIntMonitor>());
    return monitor;
}