// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Threads/NotifyPipe.h>

#include <csignal>

namespace common::signals
{
    class SignalHandlerBase
    {
    public:
        SignalHandlerBase() = default;
        SignalHandlerBase(const SignalHandlerBase&) = delete;
        SignalHandlerBase& operator=(const SignalHandlerBase&) = delete;

        virtual ~SignalHandlerBase() = default;

        int monitorFd();
        int setSignalHandler(int signal, __sighandler_t handler);
        void clearSignalHandler(int signal);
    protected:
        Common::Threads::NotifyPipe m_pipe;
    };
}
