// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SignalHandlerBase.h"

#include "../Logger.h"

using namespace common::signals;

int SignalHandlerBase::monitorFd()
{
    return m_pipe.readFd();
}
int SignalHandlerBase::setSignalHandler(int signal, __sighandler_t signal_handler)
{
    // Setup signal handler
    struct sigaction action{};
    action.sa_handler = signal_handler;
    action.sa_flags = SA_RESTART;
    int ret = ::sigaction(signal, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to setup " << signal <<" signal handler");
        return -1;
    }
    return m_pipe.writeFd();
}

void SignalHandlerBase::clearSignalHandler(int signal)
{
    struct sigaction action{};
    action.sa_handler = SIG_IGN;
    action.sa_flags = 0;
    int ret = ::sigaction(signal, &action, nullptr);
    if (ret != 0)
    {
        LOGERROR("Failed to clear " << signal <<" signal handler");
    }
}
