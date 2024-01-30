// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "SigUSR1Monitor.h"
// Package
#include "SignalHandlerTemplate.h"
#include "../Logger.h"
// Component

// Std C++
#include <csignal>

namespace common::signals
{
    namespace {
        int GL_USR1_MONITOR_PIPE = -1;
    }

    bool SigUSR1Monitor::triggered()
    {
        while (m_pipe.notified())
        {
        }

        LOGINFO("Reload triggered by USR1");

        m_reloader->update();
        return true;
    }

    SigUSR1Monitor::SigUSR1Monitor(IReloadablePtr reloadable)
        : SignalHandlerBase(SIGUSR1),
        m_reloader(std::move(reloadable))
    {
        LOGINFO("Starting USR1 monitor");
        // Setup signal handler
        GL_USR1_MONITOR_PIPE = setSignalHandler(common::signals::signal_handler<GL_USR1_MONITOR_PIPE>, true);
    }

    SigUSR1Monitor::~SigUSR1Monitor()
    {
        // clear signal handler
        GL_USR1_MONITOR_PIPE = -1;
        clearSignalHandler();
    }
}