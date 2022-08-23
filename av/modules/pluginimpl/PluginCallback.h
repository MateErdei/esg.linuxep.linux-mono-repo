/******************************************************************************************************

Copyright 2018-2022 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "HealthStatus.h"
#include "QueueTask.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/FileSystem/IFilePermissions.h>
#include <Common/FileSystem/IFileSystem.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <common/PluginUtils.h>

#include <atomic>

namespace datatypes {
        class ISystemCallWrapper;
}

namespace Plugin
{
    class PluginCallback : public virtual Common::PluginApi::IPluginCallbackApi
    {
    public:
        explicit PluginCallback(std::shared_ptr<QueueTask> task);

        void applyNewPolicy(const std::string& policyXml) override;

        void queueAction(const std::string& actionXml) override;

        void onShutdown() override;
        Common::PluginApi::StatusInfo getStatus(const std::string& appId) override;

        std::string getTelemetry() override;
        std::string getHealth() override;

        long calculateHealth(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);
        static std::pair<unsigned long, unsigned long> getThreatScannerProcessinfo(const std::shared_ptr<datatypes::ISystemCallWrapper>& sysCalls);

        void sendStatus(const std::string& revID);
        void setRunning(bool running);
        bool isRunning();
        void setSXL4Lookups(bool sxl4Lookup);
        void setThreatHealth(E_HEALTH_STATUS threatStatus);
        [[nodiscard]] long getThreatHealth() const;

    private:
        std::string generateSAVStatusXML();
        static unsigned long getIdeCount();
        static std::string getLrDataHash();
        static std::string getMlLibHash();
        static std::string getMlModelVersion();
        static std::string getVirusDataVersion();
        bool isProcessHealthy(int pid,
                              const std::string& processName,
                              const std::string& processUsername,
                              Common::FileSystem::IFileSystem* fileSystem);

        static int getProcessPidFromFile(Common::FileSystem::IFileSystem* fileSystem, const Path&);
        [[nodiscard]] bool shutdownFileValid() const;

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

        std::shared_ptr<QueueTask> m_task;
        Common::PluginApi::StatusInfo m_statusInfo;
        std::string m_revID;
        std::atomic_bool m_running = false;
        bool m_lookupEnabled = true;
        int m_allowedShutdownTime = 60;
        long m_threatStatus = E_THREAT_HEALTH_STATUS_GOOD;
    };
} // namespace Plugin
