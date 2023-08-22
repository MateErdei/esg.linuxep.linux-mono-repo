/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <string>

namespace ManagementAgent::PluginCommunication
{
    class IPolicyReceiver
    {
    public:
        virtual ~IPolicyReceiver() = default;

        virtual bool receivedGetPolicyRequest(const std::string& appId, const std::string& pluginName) = 0;
    };
} // namespace ManagementAgent::PluginCommunication
