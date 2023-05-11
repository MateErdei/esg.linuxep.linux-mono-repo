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

bool ExclusionCache::checkExclusions(const std::string& filePath) const
{
    std::lock_guard<std::mutex> lock(m_exclusionsLock);
    auto fsFilePath = fs::path(filePath);
    for (const auto& exclusion: m_exclusions)
    {
        if (exclusion.appliesToPath(fsFilePath))
        {
            LOGTRACE("File access on " << filePath << " will not be scanned due to exclusion: "  << exclusion.displayPath());
            // Can't cache user-created exclusions, since
            // files can be created under the exclusion then moved elsewhere.
            return true;
        }
    }
    return false;
}