// Copyright 2023 Sophos Limited. All rights reserved.
#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "common/ExclusionList.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace sophos_on_access_process::fanotifyhandler
{
    class ExclusionCache
    {
    public:
        virtual ~ExclusionCache() = default;

        bool setExclusions(const common::ExclusionList& exclusions);
        bool setExclusions(common::ExclusionList::list_type exclusions);
        bool checkExclusions(const std::string& filePath) const;

    protected:
        using cache_t = std::unordered_map<std::string, bool>;
        using clock_t = std::chrono::steady_clock;
        virtual bool checkExclusionsUncached(const std::string& filePath) const;
        std::chrono::milliseconds cache_lifetime_ = std::chrono::seconds(5);
        
    TEST_PUBLIC:
        common::ExclusionList m_exclusions;
    private:
        mutable std::mutex m_exclusionsLock;
        mutable cache_t cache_;
        mutable clock_t::time_point cache_time_ = clock_t::now();
    };
}
