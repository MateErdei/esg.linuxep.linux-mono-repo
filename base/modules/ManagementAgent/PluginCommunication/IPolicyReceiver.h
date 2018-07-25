/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef EVEREST_BASE_IPOLICYRECEIVER_H
#define EVEREST_BASE_IPOLICYRECEIVER_H

#include <string>

namespace ManagementAgent
{
    namespace PluginCommunication
    {
        class IPolicyReceiver
        {
        public:
            virtual ~IPolicyReceiver() = default;

            virtual bool receivedGetPolicyRequest(const std::string& appId, const std::string& policyId) = 0;
        };
    }
}

#endif //EVEREST_BASE_IPOLICYRECEIVER_H
