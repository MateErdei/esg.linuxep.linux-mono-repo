/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include "PluginCallback.h"
#include "ITaskQueue.h"

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
            Common::DirectoryWatcher::IiNotifyWrapperPtr = nullptr
            );
        void mainLoop();
        ~PluginAdapter();

    private:
        void processAction(const std::string& actionXml);

        std::future<void> m_monitor;
        std::shared_ptr<ITaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        Common::DirectoryWatcher::IiNotifyWrapperPtr m_iiNotifyWrapperPtr;

    };


} // namespace RemoteDiagnoseImpl
