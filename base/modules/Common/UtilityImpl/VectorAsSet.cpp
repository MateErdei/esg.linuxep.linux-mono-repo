/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "VectorAsSet.h"

#include <algorithm>
namespace Common
{
    namespace UtilityImpl
    {
        void VectorAsSet::setEntries(std::vector<std::string> entries)
        {
            std::sort(entries.begin(), entries.end());
            auto last = std::unique(entries.begin(), entries.end());
            m_entries = std::vector<std::string>(entries.begin(), last);
        }

        bool VectorAsSet::hasEntry(const std::string& entry) const
        {
            return std::binary_search(m_entries.begin(), m_entries.end(), entry);
        }

        const std::vector<std::string>& VectorAsSet::entries() const { return m_entries; }

    } // namespace UtilityImpl
} // namespace Common