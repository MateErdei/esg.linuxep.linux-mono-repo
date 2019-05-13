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

            virtual void addProcessToMonitor(Common::ProcessMonitoring::IProcessProxyPtr processProxyPtr) = 0;

            virtual void addReplierSocketAndHandleToPoll(Common::ZeroMQWrapper::ISocketReplier* socketReplier,
                                                         std::function<void(void)> socketHandleFunction) = 0;

            virtual int start() = 0;

        };
        using IProcessMonitorPtr = std::unique_ptr<IProcessMonitor>;
        extern IProcessMonitorPtr createProcessMonitor();
    }
}