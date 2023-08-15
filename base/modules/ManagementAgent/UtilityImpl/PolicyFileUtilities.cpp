// Copyright 2018-2023 Sophos Limited. All rights reserved.

#include "PolicyFileUtilities.h"

#include "Common/FileSystem/IFileSystem.h"
#include "Common/UtilityImpl/RegexUtilities.h"
#include "Common/UtilityImpl/StringUtils.h"

namespace ManagementAgent
{
    namespace UtilityImpl
    {
        std::string extractAppIdFromPolicyFile(const std::string& policyPath)
        {
            if (policyPath.find("flags") != std::string::npos)
            {
                return "FLAGS";
            }
            std::string policyFileName = Common::FileSystem::basename(policyPath);
            if (Common::UtilityImpl::StringUtils::startswith(policyFileName, "internal"))
            {
                if (Common::UtilityImpl::StringUtils::endswith(policyFileName, ".json"))
                {
                    std::string PolicyPattern{ R"sophos(internal_([\w]+).json)sophos" };
                    return Common::UtilityImpl::returnFirstMatch(PolicyPattern, policyFileName);
                }
                std::string PolicyPattern{ R"sophos(internal_([\w]+).xml)sophos" };
                return Common::UtilityImpl::returnFirstMatch(PolicyPattern, policyFileName);
            }
            std::string PolicyPattern{ R"sophos(([\w]+)(-[\w]+)?_policy.xml)sophos" };
            return Common::UtilityImpl::returnFirstMatch(PolicyPattern, policyFileName);
        }

    } // namespace UtilityImpl
} // namespace ManagementAgent
