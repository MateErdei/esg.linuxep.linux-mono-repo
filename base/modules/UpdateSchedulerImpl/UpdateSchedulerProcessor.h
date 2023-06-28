// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#include "Common/PersistentValue/PersistentValue.h"
#include "Common/Policy/ALCPolicy.h"
#include "Common/PluginApiImpl/BaseServiceAPI.h"
#include "Common/UtilityImpl/TimeUtils.h"

#include <UpdateScheduler/IAsyncSulDownloaderRunner.h>
#include <UpdateScheduler/ICronSchedulerThread.h>
#include <UpdateScheduler/SchedulerTaskQueue.h>
#include <UpdateSchedulerImpl/SchedulerPluginCallback.h>
#include <UpdateSchedulerImpl/configModule/UpdatePolicyTranslator.h>

namespace UpdateSchedulerImpl
{

    /**
     * Persists a list of feature codes to disk in JSON format. Used by Update Scheduler so that it can correctly
     * generate ALC status feature code list on an update failure or when first started.
     * @param Reference to std::vector<std::string> which holds list of feature codes, e.g. CORE
     */
    void writeInstalledFeatures(const std::vector<std::string>& features);

    /**
     * Returns the list of features that are currently installed, if there is no file or the file cannot be parsed
     * then this returns an empty list.
     * @return std::vector<std::string> of feature codes, e.g. CORE
     */
    std::vector<std::string> readInstalledFeatures();

    /*
     * Returns true if suldownloader is running (based on its lock file), false if not.
     */
    bool isSuldownloaderRunning();

    class DetectRequestToStop : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    class UpdateSchedulerProcessor
    {
        inline static const std::string ALC_API = "ALC";
        inline static const std::string VERSIONID = "1";
        inline static const std::string FLAGS_API = "FLAGS";

    public:
        UpdateSchedulerProcessor(
            std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> queueTask,
            std::unique_ptr<Common::PluginApi::IBaseServiceApi> baseService,
            std::shared_ptr<SchedulerPluginCallback> callback,
            std::unique_ptr<UpdateScheduler::ICronSchedulerThread> cronThread,
            std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner> sulDownloaderRunner);
        void mainLoop();
        static std::string getAppId();
        static std::string waitForPolicy(
            UpdateScheduler::SchedulerTaskQueue& queueTask,
            int maxTasksThreshold,
            const std::string& policyAppId);

    private:
        void waitForSulDownloaderToFinish(int numberOfSecondsToWait);
        void processALCPolicy(const std::string& policyXml);
        void processFlags(const std::string& flagsContent);

        void processUpdateNow(const std::string& actionXml);
        void processScheduleUpdate(bool UpdateNow = false);
        void processShutdownReceived();
        std::string processSulDownloaderFinished(
            const std::string& reportFileLocation,
            const bool processLatestReport = false);
        void processSulDownloaderFailedToStart(const std::string& errorMessage);
        void processSulDownloaderTimedOut();
        void processSulDownloaderMonitorDetached();
        void saveUpdateCacheCertificate(const std::string& cacheCertificateContent);
        void writeConfigurationData(const Common::Policy::UpdateSettings&);
        void safeMoveDownloaderReportFile(const std::string& originalJsonFilePath) const;

        void waitForSulDownloaderToFinish();
        std::shared_ptr<UpdateScheduler::SchedulerTaskQueue> m_queueTask;
        std::unique_ptr<Common::PluginApi::IBaseServiceApi> m_baseService;
        std::shared_ptr<SchedulerPluginCallback> m_callback;
        std::unique_ptr<UpdateScheduler::ICronSchedulerThread> m_cronThread;
        std::unique_ptr<UpdateScheduler::IAsyncSulDownloaderRunner> m_sulDownloaderRunner;
        configModule::UpdatePolicyTranslator m_policyTranslator;
        std::string m_updateVarPath;
        std::string m_reportfilePath;
        std::string m_configfilePath;
        std::string m_previousConfigFilePath;
        std::string m_machineID;
        Common::UtilityImpl::FormattedTime m_formattedTime;
        bool m_policyReceived;
        bool m_pendingUpdate;
        bool m_flagsPolicyProcessed;
        bool m_forceUpdate;
        bool m_forcePausedUpdate;
        Common::Policy::WeekDayAndTimeForDelay weeklySchedule_;
        std::vector<std::string> m_featuresInPolicy;
        std::vector<std::string> m_featuresCurrentlyInstalled;
        std::vector<std::string> m_subscriptionRigidNamesInPolicy;
        inline static int QUEUE_TIMEOUT = 5;
    };
} // namespace UpdateSchedulerImpl
