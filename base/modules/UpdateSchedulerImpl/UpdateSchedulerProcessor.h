/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once
#include "SchedulerPluginCallback.h"

#include "configModule/UpdatePolicyTranslator.h"

#include <Common/PluginApiImpl/BaseServiceAPI.h>
#include <Common/UtilityImpl/TimeUtils.h>
#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateScheduler/ICronSchedulerThread.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>

namespace UpdateSchedulerImpl
{
    class UpdateSchedulerProcessor
    {
        static std::string ALC_API;
        static std::string VERSIONID;

    public:
        UpdateSchedulerProcessor(
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<SchedulerPluginCallback> callback,
            std::unique_ptr<UpdateScheduler::ICronSchedulerThread> cronThread,
            std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner> sulDownloaderRunner);
        void mainLoop();
        static std::string getAppId();

    private:
        void enforceSulDownloaderFinished(int numberOfSeconds2Wait);
        void processPolicy(const std::string& policyXml);

        void processUpdateNow(const std::string& actionXml);
        void processScheduleUpdate();
        void processShutdownReceived();
        std::string processSulDownloaderFinished(const std::string& reportFileLocation);
        void processSulDownloaderFailedToStart(const std::string& errorMessage);
        void processSulDownloaderTimedOut();
        void processSulDownloaderMonitorDetached();
        void saveUpdateCacheCertificate(const std::string& cacheCertificateContent);
        void writeConfigurationData(const SulDownloader::suldownloaderdata::ConfigurationData&);

        void safeMoveDownloaderReportFile(const std::string& originalJsonFilePath) const;

        void ensureSulDownloaderNotRunning();

        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<SchedulerPluginCallback> m_callback;
        std::unique_ptr<UpdateScheduler::ICronSchedulerThread> m_cronThread;
        std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner> m_sulDownloaderRunner;
        configModule::UpdatePolicyTranslator m_policyTranslator;
        std::string m_reportfilePath;
        std::string m_configfilePath;
        std::string m_machineID;
        Common::UtilityImpl::FormattedTime m_formattedTime;
        bool m_policyReceived;
        bool m_pendingUpdate;
    };
} // namespace UpdateSchedulerImpl
