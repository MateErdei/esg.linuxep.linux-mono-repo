/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SystemConfigurator.h"

#include <string>
#include <tuple>
#include <vector>
namespace Plugin
{
    class OsqueryConfigurator
    {
    public:
        static bool ALCContainsMTRFeature(const std::string& alcPolicyXMl);

        bool enableAuditDataCollection() const;

        void loadALCPolicy(const std::string& alcPolicy);
        void prepareSystemForPlugin(bool xdrEnabled);
        static void regenerateOSQueryFlagsFile(
            const std::string& osqueryFlagsFilePath,
            bool enableAuditEventCollection,
            bool xdrEnabled);
        static void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);
        static void enableQueryPack(const std::string& queryPackFilePath);
        static void disableQueryPack(const std::string& queryPackFilePath);
        static void addTlsServerCertsOsqueryFlag(std::vector<std::string>& flags);

    protected:
        bool MTRBoundEnabled() const;
        bool disableSystemAuditDAndTakeOwnershipOfNetlink() const;
    private:
        // make it virtual to allow for not using it in tests as they require file access.
        virtual bool retrieveDisableAuditFlagFromSettingsFile() const;

        bool m_mtrboundEnabled = true;
    };
} // namespace Plugin
