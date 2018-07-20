///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#include "StatusCache.h"

bool ManagementAgent::StatusReceiverImpl::StatusCache::statusChanged(const std::string& appid,
                                                                          const std::string& statusForComparison)
{
    auto search = m_statusCache.find(appid);
    if (search == m_statusCache.end())
    {
        return updateStatus(appid,statusForComparison);
    }
    if (search->second == statusForComparison)
    {
        return false;
    }
    return updateStatus(appid, statusForComparison);
}

bool ManagementAgent::StatusReceiverImpl::StatusCache::updateStatus(const std::string& appid,
                                                                         const std::string& statusForComparison)
{
    m_statusCache[appid] = statusForComparison;
    return true;
}
