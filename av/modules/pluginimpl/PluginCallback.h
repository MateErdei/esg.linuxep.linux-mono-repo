/******************************************************************************************************

Copyright 2018-2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "QueueTask.h"

#include "datatypes/sophos_filesystem.h"

#include <Common/FileSystem/IFileSystem.h>
#include <Common/PluginApi/IPluginCallbackApi.h>
#include <common/PluginUtils.h>

#include <atomic>

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
        void sendThreatHealth(const std::string& healthJson) override;
        long calculateHealth();

        void sendStatus(const std::string& revID);
        void setRunning(bool running);
        bool isRunning();
        void setSXL4Lookups(bool sxl4Lookup);

    private:
        std::string generateSAVStatusXML();
        unsigned long getIdeCount();
        std::string getLrDataHash();
        std::string getMlLibHash();
        std::string getMlModelVersion();
        std::string getVirusDataVersion();
        bool shutdownFileValid();

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
        int m_allowedShutdownTime = 15;
    };
}; // namespace Plugin
