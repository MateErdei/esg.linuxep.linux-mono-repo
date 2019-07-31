/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/Process/IProcessInfo.h>

#include <chrono>

namespace Common
{
    namespace ProcessMonitoring
    {
        class IProcessProxy
        {
        public:
            virtual ~IProcessProxy() = default;

            /**
             * Stops the process if it is running.
             */
            virtual void stop() = 0;

            virtual std::chrono::seconds checkForExit() = 0;

            /**
             * If process is enabled, and is not running, and enough time has passed, start process.
             * If process is enabled, and is not running, and enough time has not passed, return amount of time to wait.
             * If process is disabled, and is running, stop process.
             *
             * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
             */
            virtual std::chrono::seconds ensureStateMatchesOptions() = 0;

            virtual void setEnabled(bool enabled) = 0;
        };
        using IProcessProxyPtr = std::unique_ptr<IProcessProxy>;
        extern IProcessProxyPtr createProcessProxy(Common::Process::IProcessInfoPtr processInfoPtr);
    } // namespace ProcessMonitoring
} // namespace Common