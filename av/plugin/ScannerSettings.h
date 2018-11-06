/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "IPluginCallbackApi.h"
#include <string>
#include <vector>

namespace Example
{
    struct Policy
    {
        Policy(): revId(), exclusions(), onAccess(false), scheduled(false){}
        std::string revId;
        std::vector<std::string> exclusions;
        bool onAccess;
        bool scheduled;

    };

    class ScannerSettings {
        Policy m_policy;
        void logSettings();
    public:
        void ProcessPolicy(const std::string & policyXml);
        Common::PluginApi::StatusInfo getStatus() const;
        bool isEnabled() const;
        std::vector<std::string> getExclusions() const;
    };
}

