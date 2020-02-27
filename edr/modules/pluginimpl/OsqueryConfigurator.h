/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SystemConfigurator.h"

#include <string>
#include <tuple>
namespace Plugin
{
    class OsqueryConfigurator
    {
    public:
        static bool ALCContainsMTRFeature(const std::string& alcPolicyXMl);

        bool enableAuditDataCollection() const;

        void loadALCPolicy(const std::string& alcPolicy);
        void prepareSystemForPlugin();

    protected:
        bool MTRBoundEnabled() const;
        bool disableSystemAuditDAndTakeOwnershipOfNetlink() const;
        void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection);

    private:
        // make it virtual to allow for not using it in tests as they require file access.
        virtual bool retrieveDisableAuditFlagFromSettingsFile() const;

        void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);

        bool m_mtrboundEnabled = true;
    };
} // namespace Plugin
