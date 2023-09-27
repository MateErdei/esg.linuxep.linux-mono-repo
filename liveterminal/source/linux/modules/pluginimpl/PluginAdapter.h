/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <stdio.h>
#include "PluginCallback.h"
#include "QueueTask.h"
#include "PluginUtils.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/FileSystem/IFileSystem.h>
#include <sessionrunner/ISessionRunner.h>
#include <sessionrunner/ParallelSessionProcessor.h>

namespace Plugin
{
    class PluginAdapter
    {
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        sessionrunner::ParallelSessionProcessor m_parallelSessionProcessor;

    public:
        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback);
        void mainLoop();
        static void updateSessionConnectionDataTelemetry(const std::string& url, const std::string& hostname);

    protected:
        /**
        * Convert live response action XML into JSON file for agent.
        * @param actionXml, correlationId
        * @return filename (not full path) of the JSON file created, "" if error.
        * @note This function is specifically for this action and knows only about the URL and Thumbprint.
        */
        std::string createJsonFile(const std::string& actionXml, const std::string& correlationId);

        std::tuple<std::string, std::string> readXml(const std::string& xml);

        void startLiveResponseSession(const std::string& id);

        // Protected to make createJsonFile unit-testable
        std::string m_mcsConfigPath;
    };
} // namespace Plugin
