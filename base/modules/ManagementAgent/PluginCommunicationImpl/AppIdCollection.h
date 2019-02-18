/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <UtilityImpl/VectorAsSet.h>

#include <memory>

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        using VectorAsSet = Common::UtilityImpl::VectorAsSet;
        class AppIdCollection
        {
            VectorAsSet m_policySet;
            VectorAsSet m_statusSet;

        public:
            AppIdCollection() = default;
            void setAppIdsForPolicyAndActions(std::vector<std::string> appIds);
            void setAppIdsForStatus(std::vector<std::string> appIds);
            const std::vector<std::string>& statusAppIds() const;
            bool usePolicyId(const std::string& appId) const;
            bool implementActionId(const std::string& appId) const;
            bool implementStatus(const std::string& appId) const;
        };
    } // namespace PluginCommunicationImpl
} // namespace ManagementAgent
