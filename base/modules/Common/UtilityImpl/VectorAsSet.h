/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once
#include <string>
#include <vector>

namespace Common::UtilityImpl
{
    class VectorAsSet
    {
        std::vector<std::string> m_entries;

    public:
        VectorAsSet() = default;

        void setEntries(std::vector<std::string> entries);

        bool hasEntry(const std::string& entry) const;

        const std::vector<std::string>& entries() const;
    };
} // namespace Common::UtilityImpl