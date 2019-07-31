/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <unordered_set>
#include <vector>
namespace Common
{
    namespace UtilityImpl
    {
        template<typename T>
        class OrderedSet
        {
            std::vector<T> m_orderedElements;
            std::unordered_set<T> m_uniqueElements;

        public:
            void addElement(const T& element)
            {
                auto it = m_uniqueElements.find(element);
                if (it == m_uniqueElements.end())
                {
                    m_uniqueElements.insert(element);
                    m_orderedElements.push_back(element);
                }
            }
            std::vector<T> orderedElements() const { return m_orderedElements; }
        };

    } // namespace UtilityImpl
} // namespace Common
