// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SignalHandlerBase.h"

#include "../Logger.h"

using namespace common::signals;

int SignalHandlerBase::monitorFd()
{
    return m_pipe.readFd();
}

int SignalHandlerBase::setSignalHandler(__sighandler_t signal_handler, bool restartSyscalls)
{
    // Setup signal handler
    struct sigaction action{};
    action.sa_handler = signal_handler;

    if (restartSyscalls)
    {
        action.sa_flags = SA_RESTART;
    }
    else
    {
        action.sa_flags = 0;
    }

    auto ret = ::sigaction(m_signalNumber, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to setup " << m_signalNumber <<" signal handler");
        return -1;
    }
    return m_pipe.writeFd();
}

void SignalHandlerBase::clearSignalHandler() const
{
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    action.sa_flags = 0;
    int ret = ::sigaction(m_signalNumber, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to clear " << m_signalNumber <<" signal handler");
    }
}
