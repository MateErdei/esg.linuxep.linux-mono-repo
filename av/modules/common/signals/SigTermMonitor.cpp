// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SigTermMonitor.h"
// Package
#include "SignalHandlerTemplate.h"

#include "common/Logger.h"
// Std C++
#include <csignal>

namespace common
{
    namespace
    {
        int SIGTERM_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    }

    SigTermMonitor::SigTermMonitor(bool restartSyscalls)
        : LatchingSignalHandler(SIGTERM)
    {
        // Setup signal handler
        SIGTERM_MONITOR_PIPE = setSignalHandler(common::signals::signal_handler<SIGTERM_MONITOR_PIPE>, restartSyscalls);
    }

    SigTermMonitor::~SigTermMonitor()
    {
        // clear signal handler
        SIGTERM_MONITOR_PIPE = -1;
        clearSignalHandler();
    }

    std::shared_ptr<SigTermMonitor> SigTermMonitor::getSigTermMonitor(bool restartSyscalls)
    {
        static std::shared_ptr<SigTermMonitor> monitor(std::make_shared<SigTermMonitor>(restartSyscalls));
        return monitor;
    }
} // namespace common