// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "modules/Common/Process/IProcess.h"
#include <sys/types.h>

#include <atomic>
#include <functional>
#include <mutex>
namespace Common
{
    namespace ProcessImpl
    {
        class IProcessHolder
        {
        public:
            virtual ~IProcessHolder(){};

            virtual int pid() = 0;

            virtual void wait() = 0;

            virtual Process::ProcessStatus wait(std::chrono::milliseconds timeToWait) = 0;

            virtual int exitCode() = 0;
            virtual int nativeExitCode() = 0;

            virtual std::string output() = 0;
            virtual std::string stderroutput() = 0;
            virtual std::string stdoutput() = 0;

            virtual bool hasFinished() = 0;

            virtual void sendTerminateSignal() = 0;

            virtual void sendAbortSignal() = 0;

            virtual void sendUsr1Signal() = 0;

            virtual void kill() = 0;
        };
    } // namespace ProcessImpl
} // namespace Common