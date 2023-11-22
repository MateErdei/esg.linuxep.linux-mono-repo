// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginCallback.h"
#include "TaskQueue.h"

#include "Common/PluginApi/IBaseServiceApi.h"

namespace Plugin
{
    class PluginAdapter
    {
    public:
        PluginAdapter(
                std::shared_ptr<TaskQueue> queueTask,
                std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                std::shared_ptr<PluginCallback> callback);
        void mainLoop();

    private:
        void processAction(const std::string& actionXml);

        std::shared_ptr<TaskQueue> m_taskQueue;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;

    protected:
        std::chrono::milliseconds QUEUE_TIMEOUT = std::chrono::seconds{5};

    };
} // namespace Plugin
