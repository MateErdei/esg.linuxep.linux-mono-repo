/******************************************************************************************************

Copyright 2018-2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "PluginCallback.h"
#include "QueueTask.h"
#include <livequery/IQueryProcessor.h>
#include <livequery/IResponseDispatcher.h>

#include <Common/PluginApi/IBaseServiceApi.h>

namespace Plugin
{
    class PluginAdapter
    {
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        std::unique_ptr<livequery::IQueryProcessor> m_queryProcessor;
        std::unique_ptr<livequery::IResponseDispatcher> m_responseDispatcher;

    public:

        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
            std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher);

        void mainLoop();

    private:
        void processQuery(const std::string & query, const std::string & correlationId);
        void processPolicy(const std::string& policyXml);
    };
} // namespace Plugin
