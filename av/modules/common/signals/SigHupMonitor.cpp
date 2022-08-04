// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SigHupMonitor.h"
// Package
#include "SignalHandlerTemplate.h"

#include "common/Logger.h"
// Std C++
#include <csignal>

static int SIGHUP_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

common::SigHupMonitor::SigHupMonitor(bool restartSyscalls)
    : LatchingSignalHandler(SIGHUP)
{
    // Setup signal handler
    SIGHUP_MONITOR_PIPE = setSignalHandler(common::signals::signal_handler<SIGHUP_MONITOR_PIPE>, restartSyscalls);
}

common::SigHupMonitor::~SigHupMonitor()
{
    // clear signal handler
    SIGHUP_MONITOR_PIPE = -1;
    clearSignalHandler();
}

std::shared_ptr<common::SigHupMonitor> common::SigHupMonitor::getSigHupMonitor(bool restartSyscalls)
{
    static std::shared_ptr<common::SigHupMonitor> monitor(std::make_shared<common::SigHupMonitor>(restartSyscalls));
    return monitor;
}
