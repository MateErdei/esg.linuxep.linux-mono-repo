// Copyright 2018-2023 Sophos Limited. All rights reserved.

#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "HealthStatus.h"
#include "TaskQueue.h"
#include "Telemetry.h"

#include "datatypes/sophos_filesystem.h"

#include "Common/FileSystem/IFilePermissions.h"
#include "Common/FileSystem/IFileSystem.h"
#include "Common/PluginApi/IPluginCallbackApi.h"
#include "common/PluginUtils.h"

#include <atomic>

namespace Common {
    namespace SystemCallWrapper {
        class ISystemCallWrapper;
    }
}

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<TaskQueue> task);

        void applyNewPolicy(const std::string& policyXml) override;
        void applyNewPolicyWithAppId(const std::string& appId, const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;

        long calculateHealth(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);

        /**
         * Change the revID of the SAV policy and send if it's changed.
         * @param revID
         */
        bool sendStatus(const std::string& revID);

        /**
         * Send a SAV Status, if it has changed.
         * @return True if the status has changed.
         */
        bool sendStatus();

        void setRunning(bool running);
        bool isRunning();
        void setSXL4Lookups(bool sxl4Lookup);
        void setThreatHealth(E_HEALTH_STATUS threatStatus);
        [[nodiscard]] long getThreatHealth() const;

        void setOnAccessEnabled(bool isEnabled);

    private:
        std::string generateSAVStatusXML();

        [[nodiscard]] bool shutdownFileValid(Common::FileSystem::IFileSystem* fileSystem) const;

        /**
         * Takes a linux process status file's contents and returns a value at of a
         * given key.
         *
         * As the status file is human readable, this function strips all whitespace
         * following the key until it gets a non-whitespace char.
         *
         * e.g.
         * Value in status file:
         * Uid:     1000    1000    1000    1000
         * Returns:
         * "1000    1000    1000    1000"
         *
         * @param procStatusContent is the contents of a linux process' status file
         * @param key of the value being extracted
         * @return value at key, or empty string
         */
        static std::string extractValueFromProcStatus(const std::string& procStatusContent, const std::string& key);

        std::shared_ptr<TaskQueue> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::string m_revID;
        std::string m_savStatus;
        std::atomic_bool m_running = false;
        std::atomic_bool m_onAccessEnabled = false;
        bool m_lookupEnabled = true;
        int m_allowedShutdownTime = 60;
        long m_threatStatus = E_THREAT_HEALTH_STATUS_GOOD;
TEST_PUBLIC:
        long m_serviceHealth = E_HEALTH_STATUS_GOOD;
        long m_threatDetectorServiceStatus = E_HEALTH_STATUS_GOOD;
        long m_soapServiceStatus = E_HEALTH_STATUS_GOOD;
        long m_safestoreServiceStatus = E_HEALTH_STATUS_GOOD;

        long calculateThreatDetectorHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);
        void calculateSoapHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);
        void calculateSafeStoreHealthStatus(const std::shared_ptr<Common::SystemCallWrapper::ISystemCallWrapper>& sysCalls);
    };
    using PluginCallbackSharedPtr = std::shared_ptr<PluginCallback>;
} // namespace Plugin
