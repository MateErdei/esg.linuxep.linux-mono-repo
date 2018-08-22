/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/PluginApiImpl/PluginApiImpl.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include "SchedulerPluginCallback.h"
#include <UpdateScheduler/ICronSchedulerThread.h>
#include "configModule/UpdatePolicyTranslator.h"
#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>

namespace UpdateSchedulerImpl
{
    class UpdateSchedulerProcessor
    {
        static std::string ALC_API;
        static std::string VERSIONID;
    public:
        UpdateSchedulerProcessor(std::shared_ptr<SchedulerTaskQueue> queueTask,
                                 std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
                                 std::shared_ptr<SchedulerPluginCallback> callback,
                                 std::unique_ptr<ICronSchedulerThread> cronThread,
                                 std::unique_ptr<IAsyncSulDownloaderRunner> sulDownloaderRunner);
        void mainLoop();
    private:
        void processPolicy(const std::string & policyXml);

        void processUpdateNow(const std::string& actionXml);
        void processScheduleUpdate();
        void processShutdownReceived();
        void processSulDownloaderFinished(const std::string & reportFileLocation );
        void processSulDownloaderFailedToStart(const std::string & errorMessage);
        void processSulDownloaderTimedOut( );
        void processSulDownloaderWasAborted();
        void saveUpdateCacheCertificate(const std::string& cacheCertificateContent);
        void writeConfigurationData(const SulDownloader::ConfigurationData&);

        void safeMoveDownloaderConfigFile(const std::string& originalJsonFilePath) const;

        void ensureSulDownloaderNotRunning();
        std::shared_ptr<SchedulerTaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<SchedulerPluginCallback> m_callback;
        std::unique_ptr<ICronSchedulerThread> m_cronThread;
        std::unique_ptr<IAsyncSulDownloaderRunner> m_sulDownloaderRunner;
        UpdatePolicyTranslator m_policyTranslator;
        std::string m_reportfilePath;
        std::string m_configfilePath;
        Common::UtilityImpl::FormatedTime m_formattedTime;
        bool m_policyReceived;

    };
}
