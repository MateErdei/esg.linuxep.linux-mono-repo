// Copyright 2023 Sophos Limited. All rights reserved.
#include "ExclusionCache.h"

#include "Logger.h"

using namespace sophos_on_access_process::fanotifyhandler;

bool ExclusionCache::setExclusions(const common::ExclusionList& exclusions)
{
    std::lock_guard<std::mutex> lock(m_exclusionsLock);
    if (exclusions != m_exclusions)
    {
        m_exclusions = exclusions;
        std::string printableExclusions = m_exclusions.printable();
        LOGDEBUG("Updating on-access exclusions with: " << printableExclusions);
        cache_.clear(); // Need to clear any cached answers
        return true;
    }
    return false;
}

bool ExclusionCache::setExclusions(common::ExclusionList::list_type exclusions)
{
    return setExclusions(common::ExclusionList{std::move(exclusions), 0});
}

bool ExclusionCache::checkExclusions(const std::string& filePath) const
{
    std::lock_guard<std::mutex> lock(m_exclusionsLock);

    if (m_exclusions.empty())
    {
        return false; // no point caching if we have no exclusions
    }

    auto now = clock_t::now();
    auto age = now - cache_time_;
    if (age > cache_lifetime_)
    {
        cache_.clear();
        cache_time_ = now;
    }
    else
    {
        auto cached = cache_.find(filePath);

        if (cached != cache_.end())
        {
            return cached->second;
        }
    }

    auto result = checkExclusionsUncached(filePath);
    cache_[filePath] = result;
    return result;
}

bool ExclusionCache::checkExclusionsUncached(const std::string& filePath) const
{
    common::CachedPath path{filePath};
    if (m_exclusions.appliesToPath(path, false, true))
    {
        // Can't cache user-created exclusions, since
        // files can be created under the exclusion then moved elsewhere.
        return true;
    }
    return false;
}
