/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/


#include "PolicyFileUtilities.h"
#include <Common/FileSystem/IFileSystem.h>
#include <Common/UtilityImpl/RegexUtilities.h>

namespace ManagementAgent
{
    namespace UtilityImpl
    {
        std::string extractAppIdFromPolicyFile(
                const std::string& policyPath)
        {
            std::string policyFileName = Common::FileSystem::basename(policyPath);
            std::string PolicyPattern{R"sophos(([\w]+)(-[\w]+)?_policy.xml)sophos"};
            return Common::UtilityImpl::returnFirstMatch(PolicyPattern, policyFileName);

        }

    }
}
