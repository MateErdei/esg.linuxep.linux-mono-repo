// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#include "PluginCallback.h"

#include "Common/PluginApi/IBaseServiceApi.h"
#include "runnerModule/IAsyncDiagnoseRunner.h"
#include "taskQueue/ITaskQueue.h"

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <optional>

namespace RemoteDiagnoseImpl
{
    class PluginAdapter
    {
    public:
        PluginAdapter(
            std::shared_ptr<ITaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<IAsyncDiagnoseRunner> diagnoseRunner);
        void mainLoop();
        ~PluginAdapter();

    private:
        void processAction(const std::string& actionXml);
        void sendStartedStatus();
        void sendFinishedStatus();

        bool m_processing = false;

        std::future<void> m_monitor;
        std::string m_url;
        std::shared_ptr<ITaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<IAsyncDiagnoseRunner> m_diagnoseRunner;
    };

} // namespace RemoteDiagnoseImpl
