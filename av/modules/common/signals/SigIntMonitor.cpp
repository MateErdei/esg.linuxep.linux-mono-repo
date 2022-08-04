// Copyright 2021-2022, Sophos Limited.  All rights reserved.

#include "SigIntMonitor.h"
// Package
#include "SignalHandlerTemplate.h"

#include "common/Logger.h"
// Std C++
#include <csignal>

using namespace common::signals;

static int SIGINT_MONITOR_PIPE = -1; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

SigIntMonitor::SigIntMonitor(bool restartSyscalls)
    : LatchingSignalHandler(SIGINT)
{
    // Setup signal handler
    SIGINT_MONITOR_PIPE = setSignalHandler(signal_handler<SIGINT_MONITOR_PIPE>, restartSyscalls);
}

SigIntMonitor::~SigIntMonitor()
{
    // clear signal handler
    SIGINT_MONITOR_PIPE = -1;
    clearSignalHandler();
}

std::shared_ptr<SigIntMonitor> SigIntMonitor::getSigIntMonitor(bool restartSyscalls)
{
    static std::shared_ptr<SigIntMonitor> monitor(std::make_shared<SigIntMonitor>(restartSyscalls));
    return monitor;
}