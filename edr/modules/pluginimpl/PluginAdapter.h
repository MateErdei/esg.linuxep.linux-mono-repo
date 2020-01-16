/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IOsqueryProcess.h"
#include "PluginCallback.h"
#include "QueueTask.h"
#include <livequery/IQueryProcessor.h>
#include <livequery/IResponseDispatcher.h>

#include <Common/PluginApi/IBaseServiceApi.h>

#include <future>

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
        ~PluginAdapter();

    private:

        int MAX_LOGFILE_SIZE = 1024 * 1024;
        size_t MAX_THRESHOLD = 100;
        int QUEUE_TIMEOUT = 3600;
        void processQuery(const std::string & query, const std::string & correlationId);
        void setUpOsqueryMonitor();
        void stopOsquery();
        void rotateOsqueryLogs();
        void cleanAndSetUp();
        void databasePurge();
        std::future<void> m_monitor;
        std::shared_ptr<Plugin::IOsqueryProcess> m_osqueryProcess;
        unsigned int m_timesOsqueryProcessFailedToStart;
    };
} // namespace Plugin
