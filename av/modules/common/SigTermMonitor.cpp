// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SigTermMonitor.h"
// Package
#include "Logger.h"
#include "signals/SignalHandlerTemplate.h"
// Std C++
#include <csignal>

namespace common
{
    namespace
    {
        int SIGTERM_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    }

    SigTermMonitor::SigTermMonitor()
        : LatchingSignalHandler(SIGTERM)
    {
        // Setup signal handler
        SIGTERM_MONITOR_PIPE = setSignalHandler(common::signals::signal_handler<SIGTERM_MONITOR_PIPE>);
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