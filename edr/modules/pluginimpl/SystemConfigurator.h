/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <tuple>
namespace Plugin
{
    class SystemConfigurator
    {
    public:
        static void setupOSForAudit(bool disableAuditD);

    private:
        static bool checkIfServiceActive(const std::string& serviceName);

        static void stopAndDisableSystemService(const std::string& serviceName);

        static std::tuple<int, std::string> runSystemCtlCommand(const std::string& command, const std::string& target);

        static bool checkIfJournaldLinkedToAuditSubsystem();

        static void maskJournald();

        static void breakLinkBetweenJournaldAndAuditSubsystem();
    };
} // namespace Plugin