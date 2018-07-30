/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#ifndef MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPOLICYRECEIVER_H
#define MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPOLICYRECEIVER_H

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

#endif //MANAGEMENTAGENT_PLUGINCOMMUNICATION_IPOLICYRECEIVER_H
