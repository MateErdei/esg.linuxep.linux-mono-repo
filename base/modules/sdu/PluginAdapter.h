/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "PluginCallback.h"
#include "ITaskQueue.h"
#include "IAsyncDiagnoseRunner.h"

#include <Common/DirectoryWatcher/IiNotifyWrapper.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/Threads/AbstractThread.h>

#include <functional>
#include <future>
#include <optional>
#include <map>
#include <memory>

namespace RemoteDiagnoseImpl
{
    class PluginAdapter
    {
    public:
        PluginAdapter(
            std::shared_ptr<ITaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<IAsyncDiagnoseRunner> diagnoseRunner
            );
        void mainLoop();
        ~PluginAdapter();

    private:
        void processAction(const std::string& actionXml);
        void sendFinishedStatus();
        void processZip();
        bool m_processing = false;

        std::future<void> m_monitor;
        std::shared_ptr<ITaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<IAsyncDiagnoseRunner> m_diagnoseRunner;

    };


} // namespace RemoteDiagnoseImpl
