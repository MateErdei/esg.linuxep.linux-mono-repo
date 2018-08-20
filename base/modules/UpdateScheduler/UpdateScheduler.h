/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApiImpl/PluginApiImpl.h>
#include "SchedulerTaskQueue.h"
#include "SchedulerPluginCallback.h"

namespace UpdateScheduler
{
    class UpdateScheduler
    {
        std::shared_ptr<SchedulerTaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<SchedulerPluginCallback> m_callback;
    public:
        UpdateScheduler(std::shared_ptr<SchedulerTaskQueue> queueTask, std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService, std::shared_ptr<SchedulerPluginCallback> callback);
        void mainLoop();
    private:
        void processPolicy(const std::string & policyXml);
    };
}
