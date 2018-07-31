/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IPolicyReceiver
        {
        public:
            virtual ~IPolicyReceiver() = default;

            virtual bool receivedGetPolicyRequest(const std::string& appId) = 0;
        };
    }
}


