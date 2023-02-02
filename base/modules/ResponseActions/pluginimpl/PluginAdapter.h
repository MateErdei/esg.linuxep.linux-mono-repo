// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"

#include <Common/PluginApi/IBaseServiceApi.h>

namespace Plugin
{
    class DetectRequestToStop : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

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
        void processAction(const std::string& policyXml);
    };
} // namespace Plugin
