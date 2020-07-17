/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <Common/UtilityImpl/VectorAsSet.h>

#include <memory>

namespace Common
{
    namespace PluginCommunicationImpl
    {
        using VectorAsSet = Common::UtilityImpl::VectorAsSet;
        class AppIdCollection
        {
            VectorAsSet m_policySet;
            VectorAsSet m_statusSet;
            VectorAsSet m_actionSet;

        public:
            AppIdCollection() = default;
            void setAppIdsForPolicy(std::vector<std::string> appIds);
            void setAppIdsForActions(std::vector<std::string> appIds);
            void setAppIdsForStatus(std::vector<std::string> appIds);
            const std::vector<std::string>& statusAppIds() const;
            bool usePolicyId(const std::string& appId) const;
            bool implementActionId(const std::string& appId) const;
            bool implementStatus(const std::string& appId) const;
        };
    } // namespace PluginCommunicationImpl
} // namespace Common
