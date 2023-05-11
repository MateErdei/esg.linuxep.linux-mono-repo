// Copyright 2023 Sophos All rights reserved.
#pragma once

#include "common/Exclusion.h"

#include <mutex>
#include <vector>

namespace sophos_on_access_process::fanotifyhandler
{
    class ExclusionCache
    {
    public:
        std::vector<common::Exclusion> m_exclusions;
        mutable std::mutex m_exclusionsLock;
    };
}
