// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ISignalHandlerBase.h"

#include "Common/Threads/NotifyPipe.h"

#include <cassert>
#include <csignal>

namespace common::signals
{
    class SignalHandlerBase : public ISignalHandlerBase
    {
    public:
        explicit SignalHandlerBase(int signalNumber)
            : m_signalNumber(signalNumber)
        {}
        SignalHandlerBase(const SignalHandlerBase&) = delete;
        SignalHandlerBase& operator=(const SignalHandlerBase&) = delete;

        int monitorFd() override;
        int setSignalHandler(__sighandler_t handler, bool restartSyscalls=false);
        void clearSignalHandler() const;
    protected:
        Common::Threads::NotifyPipe m_pipe;
        int m_signalNumber;
    };
}
