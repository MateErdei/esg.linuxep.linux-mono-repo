// Copyright 2023 Sophos All rights reserved.
#include "ExclusionCache.h"

#include "Logger.h"

using namespace sophos_on_access_process::fanotifyhandler;

bool ExclusionCache::setExclusions(const std::vector<common::Exclusion>& exclusions)
{
    std::lock_guard<std::mutex> lock(m_exclusionsLock);
    if (exclusions != m_exclusions)
    {
        m_exclusions = exclusions;
        std::stringstream printableExclusions;
        for(const auto &exclusion: exclusions)
        {
            printableExclusions << "[\"" << exclusion.displayPath() << "\"] ";
        }
        LOGDEBUG("Updating on-access exclusions with: " << printableExclusions.str());
        return true;
    }
    return false;
}
