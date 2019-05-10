/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <Common/Threads/NotifyPipe.h>

namespace Common
{
    namespace ProcessMonitoringImpl
    {
        class SignalHandler
        {
        public:
            SignalHandler();
            ~SignalHandler();

            bool clearSubProcessExitPipe();
            bool clearTerminationPipe();

            int subprocessExitFileDescriptor();
            int terminationFileDescriptor();
        };
    } // namespace watchdogimpl
} // namespace watchdog
