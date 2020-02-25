/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include "SystemConfigurator.h"
#include <string>
#include <tuple>
namespace Plugin {
    class OsqueryConfigurator {
    public:
        void prepareSystemForPlugin();

    private:
        bool retrieveDisableAuditFlagFromSettingsFile() const;
        void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection);
        void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);
    };
}
