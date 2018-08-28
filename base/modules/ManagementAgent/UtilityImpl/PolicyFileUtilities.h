/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#pragma once


#include <string>

namespace ManagementAgent
{
    namespace UtilityImpl
    {

        /**
         * The policy file pattern currently implemented by mcsrouter: GenericAdapter::__processPolicy
         * is as follow: AppID[-PolicyType]_policy.xml
         */
        std::string extractAppIdFromPolicyFile(const std::string& policyPath);
    }
}
