/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "AppIdCollection.h"

namespace Common
{
    namespace PluginCommunicationImpl
    {
        void AppIdCollection::setAppIdsForPolicy(std::vector<std::string> appIds)
        {
            m_policySet.setEntries(std::move(appIds));
        }

        void AppIdCollection::setAppIdsForActions(std::vector<std::string> appIds)
        {
            m_actionSet.setEntries(std::move(appIds));
        }

        void AppIdCollection::setAppIdsForStatus(std::vector<std::string> appIds)
        {
            m_statusSet.setEntries(std::move(appIds));
        }

        bool AppIdCollection::usePolicyId(const std::string& appId) const { return m_policySet.hasEntry(appId); }

        bool AppIdCollection::implementActionId(const std::string& appId) const { return m_actionSet.hasEntry(appId); }

        bool AppIdCollection::implementStatus(const std::string& appId) const { return m_statusSet.hasEntry(appId); }

        const std::vector<std::string>& AppIdCollection::statusAppIds() const { return m_statusSet.entries(); }

    } // namespace PluginCommunicationImpl
} // namespace Common