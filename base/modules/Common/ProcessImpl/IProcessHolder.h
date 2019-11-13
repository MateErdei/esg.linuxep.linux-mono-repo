/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcess.h>
#include <sys/types.h>

#include <functional>
#include <mutex>
#include <atomic>
namespace Common
{
    namespace ProcessImpl
    {

        class IProcessHolder
        {
        public:
            virtual ~IProcessHolder()
            {};

            virtual int pid() = 0;

            virtual void wait() = 0;

            virtual Process::ProcessStatus wait(std::chrono::milliseconds timeToWait) = 0;

            virtual int exitCode() = 0;

            virtual std::string output() = 0;

            virtual bool hasFinished() = 0;

            virtual void sendTerminateSignal() = 0;

            virtual void kill() = 0;
        };
    }
}