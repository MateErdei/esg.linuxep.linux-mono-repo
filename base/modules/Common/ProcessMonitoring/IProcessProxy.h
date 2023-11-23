// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/Process/IProcess.h"
#include "Common/Process/IProcessInfo.h"
#include "Common/Threads/NotifyPipe.h"

#include <chrono>

namespace Common::ProcessMonitoring
{
    class IProcessProxy
    {
    public:
        virtual ~IProcessProxy() = default;

        /**
         * Stops the process if it is running.
         */
        virtual void stop() = 0;

        using exit_status_t = std::pair<std::chrono::seconds, Process::ProcessStatus>;

        /**
         * Check if the process has exited, including waiting if it is supposed to stop
         * @return pair<waitPeriod, process_status> waitPeriod=min delay before checking again
         */
        virtual exit_status_t checkForExit() = 0;

        /**
         * If process is enabled, and is not running, and enough time has passed, start process.
         * If process is enabled, and is not running, and enough time has not passed, return amount of time to wait.
         * If process is disabled, and is running, stop process.
         *
         * @return How many seconds to wait before we are ready to start again, 3600 if we are running already
         */
        virtual std::chrono::seconds ensureStateMatchesOptions() = 0;

        virtual void shutDownProcessCheckForExit() = 0;

        virtual void setEnabled(bool enabled) = 0;
        virtual bool isRunning() = 0;

        [[nodiscard]] virtual std::string name() const = 0;
        virtual void setCoreDumpMode(bool mode) = 0;

        using NotifyPipeSharedPtr = std::shared_ptr<Common::Threads::NotifyPipe>;
        /**
         * Set the pipe that should be notified when the process terminates.
         * (When BoostProcessHolder callback to us)
         * @param pipe
         */
        virtual void setTerminationCallbackNotifyPipe(NotifyPipeSharedPtr pipe) = 0;
    };
    using IProcessProxyPtr = std::unique_ptr<IProcessProxy>;
    extern IProcessProxyPtr createProcessProxy(Common::Process::IProcessInfoPtr processInfoPtr);
} // namespace Common::ProcessMonitoring