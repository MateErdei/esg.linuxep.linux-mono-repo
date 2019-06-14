/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/ZeroMQWrapper/ISocketReplier.h>
#include <Common/ProcessMonitoring/IProcessProxy.h>

#include <functional>

namespace Common
{
    namespace ProcessMonitoring
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
            virtual void addReplierSocketAndHandleToPoll(Common::ZeroMQWrapper::ISocketReplier* socketReplier,
                                                         std::function<void(void)> socketHandleFunction) = 0;
            /**
             * Start the main loop of the process monitor. Loop is terminated when a termination signal is
             * received.
             * @return
             */
            virtual int run() = 0;

            /**
             * From a different thread, request the processMonitor to shutdown itself.
             * The processMonitor is allowed to take sometime to respond to this request.
             */
            virtual void requestStop() = 0;

        };
        using IProcessMonitorPtr = std::unique_ptr<IProcessMonitor>;
        extern IProcessMonitorPtr createProcessMonitor();
    }
}