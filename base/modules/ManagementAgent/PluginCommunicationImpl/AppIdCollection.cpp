//
// Created by pair on 11/07/18.
//

#include <algorithm>
#include "AppIdCollection.h"

namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {

        void VectorAsSet::setEntries(std::vector<std::string> entries)
        {
            std::sort( entries.begin(), entries.end());
            auto last = std::unique(entries.begin(), entries.end());
            m_entries = std::vector<std::string>( entries.begin(), last);
        }

        bool VectorAsSet::hasEntry(const std::string &entry) const
        {
            return std::binary_search(m_entries.begin(), m_entries.end(), entry);
        }

        const std::vector<std::string> &VectorAsSet::entries() const
        {
            return m_entries;
        }

        void AppIdCollection::setAppIdsForPolicyAndActions(std::vector<std::string> appIds)
        {
            m_policySet.setEntries(std::move(appIds));
        }

        void AppIdCollection::setAppIdsForStatus(std::vector<std::string> appIds)
        {
            m_statusSet.setEntries(std::move(appIds));
        }

        bool AppIdCollection::usePolicyId(const std::string &appId) const
        {
            return m_policySet.hasEntry(appId);
        }

        bool AppIdCollection::implementActionId(const std::string &appId) const
        {
            return m_policySet.hasEntry(appId);
        }

        bool AppIdCollection::implementStatus(const std::string &appId) const
        {
            return m_statusSet.hasEntry(appId);
        }

        const std::vector<std::string> &AppIdCollection::statusAppIds() const
        {
            return m_statusSet.entries();
        }


    }
}