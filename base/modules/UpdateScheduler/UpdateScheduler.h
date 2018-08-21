/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApiImpl/PluginApiImpl.h>
#include "SchedulerTaskQueue.h"
#include "SchedulerPluginCallback.h"
#include "ICronSchedulerThread.h"
#include "UpdatePolicyTranslator.h"

namespace UpdateScheduler
{
    class UpdateScheduler
    {
    public:
        UpdateScheduler(std::shared_ptr<SchedulerTaskQueue> queueTask,
                        std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                        std::shared_ptr<SchedulerPluginCallback> callback,
                        std::unique_ptr<ICronSchedulerThread> cronThread);
        void mainLoop();
    private:
        void processPolicy(const std::string & policyXml);
        void processUpdateNow();
        void processScheduleUpdate();
        void processShutdownReceived();
        void processSulDownloaderFinished(const std::string & reportFileLocation );
        void processSulDownloaderFailedToStart(const std::string & errorMessage);
        void processSulDownloaderTimedOut( );
        void processSulDownloaderWasAborted();


        void saveUpdateCacheCertificate(const std::string& cacheCertificateContent);

        void writeConfigurationData(const SulDownloader::ConfigurationData&);

        std::shared_ptr<SchedulerTaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<SchedulerPluginCallback> m_callback;
        std::unique_ptr<ICronSchedulerThread> m_cronThread;
        UpdatePolicyTranslator m_policyTranslator;

    };
}
