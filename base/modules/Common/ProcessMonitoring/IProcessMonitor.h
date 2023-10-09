// Copyright 2019-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/ProcessMonitoring/IProcessProxy.h"
#include "Common/ZeroMQWrapper/ISocketReplier.h"

#include <functional>

namespace Common::ProcessMonitoring
{
    class IProcessMonitor
    {
    public:
        virtual ~IProcessMonitor() = default;

        /**
         * Add a process to be monitored bu the process monitor
         * @param processProxyPtr
         */
        virtual void addProcessToMonitor(Common::ProcessMonitoring::IProcessProxyPtr processProxyPtr) = 0;
        /**
         * Add a receiver socket to be included in the main socket poll loop and a handle function
         * that is called when the socket receives data
         * @param socketReplier
         * @param socketHandleFunction
         */
        virtual void addReplierSocketAndHandleToPoll(
            Common::ZeroMQWrapper::ISocketReplier* socketReplier,
            std::function<void(void)> socketHandleFunction) = 0;
        /**
         * Start the main loop of the process monitor. Loop is terminated when a termination signal is
         * received.
         * @return
         */
        virtual int run() = 0;
    };
    using IProcessMonitorPtr = std::unique_ptr<IProcessMonitor>;
    extern IProcessMonitorPtr createProcessMonitor();
} // namespace Common::ProcessMonitoring