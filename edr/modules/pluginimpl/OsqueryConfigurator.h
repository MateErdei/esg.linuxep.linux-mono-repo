/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "MtrMonitor.h"
#include "SystemConfigurator.h"

#include <string>
#include <tuple>
#include <vector>
namespace Plugin
{
    class OsqueryConfigurator
    {
    public:
        OsqueryConfigurator();
        static bool ALCContainsMTRFeature(const std::string& alcPolicyXMl);

        bool enableAuditDataCollection() const;

        void loadALCPolicy(const std::string& alcPolicy);
        void prepareSystemForPlugin(bool xdrEnabled, time_t scheduleEpoch, bool xdrDataLimitHit);
        static void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection,
                                               bool xdrEnabled,
                                               time_t scheduleEpoch);
        static void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);
        static void enableQueryPack(const std::string& queryPackFilePath);
        static void disableQueryPack(const std::string& queryPackFilePath);
        static void addTlsServerCertsOsqueryFlag(std::vector<std::string>& flags);

        // This is polled by pluginAdapter, e.g. once a minute to check if netlink ownership has changed and therefore
        // needs to restart osquery with updated audit flags.
        bool checkIfReconfigurationRequired();

    protected:
        MtrMonitor m_mtrMonitor;
        bool m_mtrInAlcPolicy = true;
        std::optional<bool> m_mtrHasScheduledQueries = std::nullopt;

        bool getPresenceOfMtrInAlcPolicy() const;
        static bool enableAuditDataCollectionInternal(bool disableAuditDInPluginConfig, bool mtrInAlcPolicy, std::optional<bool> mtrHasScheduledQueries);

    private:
        // make it virtual to allow for not using it in tests as they require file access.
        virtual bool retrieveDisableAuditDFlagFromSettingsFile() const;
        bool m_disableAuditDInPluginConfig = true;

    };
} // namespace Plugin
