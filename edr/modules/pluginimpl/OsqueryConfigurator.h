/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <tuple>
namespace Plugin {
    class OsqueryConfigurator {
    public:
        void prepareSystemForPlugin();

    private:
        void regenerateOSQueryFlagsFile(const std::string& osqueryFlagsFilePath, bool enableAuditEventCollection);
        void regenerateOsqueryConfigFile(const std::string& osqueryConfigFilePath);
        bool checkIfServiceActive(const std::string& serviceName);
        void stopAndDisableSystemService(const std::string& serviceName);
        std::tuple<int, std::string> runSystemCtlCommand(const std::string& command, const std::string& target);
        bool checkIfJournaldLinkedToAuditSubsystem();
        void maskJournald();
        void breakLinkBetweenJournaldAndAuditSubsystem();

    };
}
