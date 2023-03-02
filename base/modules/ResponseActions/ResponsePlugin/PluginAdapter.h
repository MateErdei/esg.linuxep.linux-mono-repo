// Copyright 2023 Sophos Limited. All rights reserved.

#pragma once

#include "IActionRunner.h"
#include "PluginCallback.h"
#include "TaskQueue.h"

#include <Common/PluginApi/IBaseServiceApi.h>

#include <deque>

namespace ResponsePlugin
{
    class DetectRequestToStop : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class PluginAdapter
    {
        std::shared_ptr<TaskQueue> m_queueTask;
        std::unique_ptr<IActionRunner> m_runner;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;

    public:
        PluginAdapter(
            std::shared_ptr<TaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::unique_ptr<IActionRunner> runner,
            std::shared_ptr<PluginCallback> callback);
        void mainLoop();

    private:
        void processAction(const std::string& actionJson, const std::string& correlationId);
        std::deque<Task> m_actionQueue;
    };
} // namespace ResponsePlugin
