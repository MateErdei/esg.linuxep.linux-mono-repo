/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"
#include <Common/PluginApi/IBaseServiceApi.h>
#include "PluginCallback.h"

namespace TemplatePlugin
{
    class PluginAdapter
    {
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
    public:
        PluginAdapter(
                std::shared_ptr<QueueTask> queueTask,
                std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                std::shared_ptr<PluginCallback> callback);
        void mainLoop();
    private:
        void processPolicy(const std::string & policyXml);
    };
}
