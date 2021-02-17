/******************************************************************************************************

Copyright 2018-2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IOsqueryProcess.h"
#include "MtrMonitor.h"
#include "OsqueryConfigurator.h"
#include "OsqueryDataManager.h"
#include "PluginCallback.h"
#include "QueueTask.h"

#include <Common/PluginApi/IBaseServiceApi.h>
#include <Common/Process/IProcess.h>
#include <modules/osqueryextensions/LoggerExtension.h>
#include <modules/osqueryextensions/SophosExtension.h>
#include <queryrunner/IQueryRunner.h>
#include <queryrunner/ParallelQueryProcessor.h>

#include <future>
#include <list>

namespace Plugin
{
    class DetectRequestToStop : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
    class FailedToParseLiveQueryPolicy : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };
    class PluginAdapter
    {
        std::shared_ptr<QueueTask> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<PluginCallback> m_callback;
        queryrunner::ParallelQueryProcessor m_parallelQueryProcessor;

    public:
        /* Keeps track of all the tasks sent to the queue
         * till it finds the ALC policy. It then reinserts the other tasks into the queue.
         * This method is meant to allow for waiting for the ALC policy that must be
         * sent on the initialization of the plugin, while also dealing with edge cases where
         * this message may not come.
         * This also simplifies component tests so that they are not required to send ALC policy.
         * Parsing the ALC policy for the signs of MTR is also a temporary gap to smooth transition
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
         * It is likely to always be the first option, as it is unlikely to receive TASKS in a way faster than
         * the waitForTheFirstALCPolicy will be able to process them.
         * Also, there is not a promise that the method will not go beyond the timeoutInMs. In fact, the timeout will be
         * restored whenever a new task is received up to maxTasksThreshold tasks.
         * The method has been made static public to simplify tests.
         * */

        static std::string
        waitForTheFirstPolicy(QueueTask& queueTask, std::chrono::seconds timeoutInS, int maxTasksThreshold,
                              const std::string& policyAppId);

        std::string serializeLiveQueryStatus(bool dailyDataLimitExceeded);

        PluginAdapter(
            std::shared_ptr<QueueTask> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<PluginCallback> callback);

        void mainLoop();
        ~PluginAdapter();

        unsigned int getDataLimit(const std::string &liveQueryPolicy);
        std::string getRevId(const std::string &liveQueryPolicy);

        bool hasScheduleEpochEnded(time_t now);
        static bool haveCustomQueriesChanged(const std::optional<std::string>& customQueries);

    protected:
        /*
         * Process ALC policies received given that the plugin subscribes to ALC policy.
         * It will pass the policy to the OsqueryConfigurator and detect if Osquery needs to be restarted or not.
         * This is meant to be called once in the initialization ( firstTime=true). It assumes the osquery will still
         * be started, hence, it will not update the queue with a require restart.
         * But, on arrival of policies, (firstTime=false) it may also push to the queue a RestartRequired.
         */
        void processALCPolicy(const std::string&, bool firstTime);
        void processLiveQueryPolicy(const std::string&);
        void ensureMCSCanReadOldResponses();
        OsqueryConfigurator& osqueryConfigurator();

        std::string m_liveQueryRevId = "";
        unsigned int m_dataLimit;
        std::string m_liveQueryStatus = "NoRef";

        std::shared_ptr<LoggerExtension> m_loggerExtensionPtr;

        Common::PersistentValue<time_t> m_scheduleEpoch;
        // 6 Days in seconds
        const time_t SCHEDULE_EPOCH_DURATION = 518400;
    private:
        void innerMainLoop();
        OsqueryDataManager m_DataManager;
        size_t MAX_THRESHOLD = 100;
        int QUEUE_TIMEOUT = 5;
        bool m_isXDR = false;
        void sendLiveQueryStatus();

        // If plugin memory exceeds this limit then restart the entire plugin (100 MB)
        static const int MAX_PLUGIN_MEM_BYTES = 100000000;



        void processQuery(const std::string& query, const std::string& correlationId);
        void processFlags(const std::string& flagsContent);
        // setUpOsqueryMonitor sets up a process monitor with IOsqueryProcess, should only be called on EDR start up
        // and during restart, we should not call setUpOsqueryMonitor anywhere else to restart osquery.
        void setUpOsqueryMonitor();
        void registerAndStartExtensionsPlugin();
        void stopOsquery();
        void cleanUpOldOsqueryFiles();
        void databasePurge();
        static bool pluginMemoryAboveThreshold();
        void loadXdrFlags();
        void dataFeedExceededCallback();

        std::future<void> m_monitor;
        std::shared_ptr<Plugin::IOsqueryProcess> m_osqueryProcess;
        unsigned int m_timesOsqueryProcessFailedToStart;
        OsqueryConfigurator m_osqueryConfigurator;
        bool m_collectAuditEnabled = false;
        bool m_restartNoDelay = false;
        std::vector<Json::Value> m_foldingRules;

        std::list<std::pair<std::shared_ptr<IServiceExtension>, std::shared_ptr<std::atomic_bool>>> m_extensionAndStateList;
    };
} // namespace Plugin
