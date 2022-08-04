// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "SigIntMonitor.h"
// Package
#include "SignalHandlerTemplate.h"

#include "common/Logger.h"
// Std C++
#include <csignal>

static int SIGINT_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

common::SigIntMonitor::SigIntMonitor(bool restartSyscalls)
    : LatchingSignalHandler(SIGINT)
{
    // Setup signal handler
    SIGINT_MONITOR_PIPE = setSignalHandler(common::signals::signal_handler<SIGINT_MONITOR_PIPE>, restartSyscalls);
}

common::SigIntMonitor::~SigIntMonitor()
{
    // clear signal handler
    SIGINT_MONITOR_PIPE = -1;
    clearSignalHandler();
}

std::shared_ptr<common::SigIntMonitor> common::SigIntMonitor::getSigIntMonitor(bool restartSyscalls)
{
    static std::shared_ptr<common::SigIntMonitor> monitor(std::make_shared<common::SigIntMonitor>(restartSyscalls));
    return monitor;
}