// Copyright 2020-2022, Sophos Limited.  All rights reserved.

#include "SigHupMonitor.h"
// Package
#include "SignalHandlerTemplate.h"

#include "common/Logger.h"
// Std C++
#include <csignal>

using namespace common::signals;

static int SIGHUP_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

SigHupMonitor::SigHupMonitor(bool restartSyscalls)
    : LatchingSignalHandler(SIGHUP)
{
    // Setup signal handler
    SIGHUP_MONITOR_PIPE = setSignalHandler(signal_handler<SIGHUP_MONITOR_PIPE>, restartSyscalls);
}

SigHupMonitor::~SigHupMonitor()
{
    // clear signal handler
    SIGHUP_MONITOR_PIPE = -1;
    clearSignalHandler();
}

std::shared_ptr<SigHupMonitor> SigHupMonitor::getSigHupMonitor(bool restartSyscalls)
{
    static std::shared_ptr<SigHupMonitor> monitor(std::make_shared<SigHupMonitor>(restartSyscalls));
    return monitor;
}
