/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once


#include <string>
#include <vector>
#include <memory>
namespace ManagementAgent
{
    namespace PluginCommunicationImpl
    {
        // TODO LINUXEP-6536: move to utilities
        class VectorAsSet
        {
            std::vector<std::string> m_entries;
        public:
            VectorAsSet() = default;
            void setEntries( std::vector<std::string> entries);
            bool hasEntry( const std::string & entry) const;
            const std::vector<std::string> & entries() const;
        };

        class AppIdCollection
        {
            VectorAsSet m_policySet;
            VectorAsSet m_statusSet;
        public:
            AppIdCollection() = default;
            void setAppIdsForPolicyAndActions( std::vector<std::string> appIds);
            void setAppIdsForStatus( std::vector<std::string> appIds);
            const std::vector<std::string> & statusAppIds() const;
            bool usePolicyId( const std::string & appId) const;
            bool implementActionId( const std::string & appId) const;
            bool implementStatus( const std::string & appId) const;
        };
    }
}


