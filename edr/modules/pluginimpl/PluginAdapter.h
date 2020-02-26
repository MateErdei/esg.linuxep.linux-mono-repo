/******************************************************************************************************

Copyright 2018-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IOsqueryProcess.h"
#include "PluginCallback.h"
#include "QueueTask.h"
#include "OsqueryDataManager.h"
#include "OsqueryConfigurator.h"
#include <livequery/IQueryProcessor.h>

#include <livequery/IResponseDispatcher.h>
#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/Process/IProcess.h>

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
        /* Keeps track of all the tasks sent to the queue
         * till it finds the ALC policy. It then reinsert the other tasks into the queue.
         * This method is meant to allow for waiting for the ALC policy that must be
         * sent on the initialization of the plugin, while also dealing with edge cases where
         * this message may not come.
         * This also simplifies component tests so that they are not required to send ALC policy.
         * Parsing the ALC policy for the sign of MTR is also a temporary gap to smooth transition
         * to the EDR with scheduled capability. Hence, this method will also go away soon.
         * For this reason, some simplifications are to be accepted for this method:
         *
         * The queue will not be completely restored in relation to the relative order of the tasks.
         * For example if the queue receives:
         *   T1, T2, ALCPOLICY, T3
         * The queue after the waitForTheFirstPolicy may either be:
         *   T1, T2, T3
         *   or
         *   T3, T1, T2.
         * It is likely to always be the first option, as it is unlikely to receive TASKS in a way faster that
         * the waitForTheFirstALCPolicy will be able to process them.
         * Also, there is not promise that the method will not go beyond the timeoutInMs. In fact, the timeout will be
         * restored whenever a new task is received up to maxTasksThreshold tasks.
         * The method has been made static public to simplify test.
         * */

        static std::string waitForTheFirstALCPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS, int maxTasksThreshold);


        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback,
            std::unique_ptr<livequery::IQueryProcessor> queryProcessor,
            std::unique_ptr<livequery::IResponseDispatcher> responseDispatcher);

        void mainLoop();
        ~PluginAdapter();

    protected:

        /*
         * Process ALC policies received given that the plugin subscribes to ALC policy.
         * It will pass the policy to the OsqueryConfigurator and detect if Osquery needs to be restarted or not.
         * This is meant to be called once in the initialization ( firstTime=true). It assumes the osquery will still
         * be started, hence, it will not update the queue with a require restart.
         * But, on arrival of policies, (firstTime=false) it may also push to the queue a RestartRequired.
         */
        void processALCPolicy(const std::string &, bool firstTime);

        OsqueryConfigurator& osqueryConfigurator();
    private:
        OsqueryDataManager m_DataManager;
        size_t MAX_THRESHOLD = 100;
        int QUEUE_TIMEOUT = 3600;
        void processQuery(const std::string & query, const std::string & correlationId);
        void setUpOsqueryMonitor();
        void stopOsquery();
        void cleanUpOldOsqueryFiles();
        void databasePurge();

        std::future<void> m_monitor;
        std::shared_ptr<Plugin::IOsqueryProcess> m_osqueryProcess;
        unsigned int m_timesOsqueryProcessFailedToStart;
        OsqueryConfigurator m_osqueryConfigurator;
        bool m_collectAuditEnabled = false;
    };
} // namespace Plugin
