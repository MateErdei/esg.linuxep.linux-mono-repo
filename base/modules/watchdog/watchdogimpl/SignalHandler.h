/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once


#include <Common/Threads/NotifyPipe.h>

namespace watchdog
{
    namespace watchdogimpl
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
    }
}
