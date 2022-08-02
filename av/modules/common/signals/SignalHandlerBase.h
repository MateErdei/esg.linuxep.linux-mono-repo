// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#include <Common/Threads/NotifyPipe.h>

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
    protected:
        Common::Threads::NotifyPipe m_pipe;
        bool m_signalled = false;
    };
}
