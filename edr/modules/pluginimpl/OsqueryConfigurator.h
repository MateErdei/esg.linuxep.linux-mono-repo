/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SystemConfigurator.h"

#include <string>
#include <tuple>
#include <vector>
#include <optional>

namespace Plugin
{
    class OsqueryConfigurator
    {
    public:
        OsqueryConfigurator();

        bool shouldAuditDataCollectionBeEnabled() const;

        void prepareSystemForPlugin(bool xdrEnabled, time_t scheduleEpoch);
        static void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection,
                                               bool xdrEnabled,
                                               time_t scheduleEpoch);
        static void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);
        static void regenerateOsqueryOptionsConfigFile(const std::string& osqueryConfigFilePath, unsigned long expiry);
        static void addTlsServerCertsOsqueryFlag(std::vector<std::string>& flags);

    protected:
        bool m_disableAuditDInPluginConfig = true;

    private:
        // make it virtual to allow for not using it in tests as they require file access.
        virtual bool retrieveDisableAuditDFlagFromSettingsFile() const;

    };
} // namespace Plugin
