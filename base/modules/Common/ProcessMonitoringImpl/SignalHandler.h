// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include <Common/ProcessMonitoring/ISignalHandler.h>

#include <Common/Threads/NotifyPipe.h>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        class SignalHandler : public ISignalHandler
        {
        public:
            SignalHandler();
            ~SignalHandler();

            bool clearSubProcessExitPipe() override;
            bool clearTerminationPipe() override;
            bool clearUSR1Pipe() override;

            int subprocessExitFileDescriptor() override;
            int terminationFileDescriptor() override;
            int usr1FileDescriptor() override;
        };
    } // namespace ProcessMonitoringImpl
} // namespace Common
