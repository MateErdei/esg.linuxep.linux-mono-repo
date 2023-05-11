// Copyright 2023 Sophos All rights reserved.
#pragma once

#ifndef TEST_PUBLIC
# define TEST_PUBLIC private
#endif

#include "common/Exclusion.h"

#include <mutex>
#include <vector>

namespace sophos_on_access_process::fanotifyhandler
{
    class ExclusionCache
    {
    public:
        bool setExclusions(const std::vector<common::Exclusion>& exclusions);
        bool checkExclusions(const std::string& filePath) const;

    TEST_PUBLIC:
        std::vector<common::Exclusion> m_exclusions;
    private:
        mutable std::mutex m_exclusionsLock;
    };
}
