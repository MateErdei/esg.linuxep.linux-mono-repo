/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "StatusCache.h"

using namespace ManagementAgent::StatusCache;

namespace ManagementAgent
{
    namespace StatusCacheImpl
    {

        bool StatusCache::statusChanged(const std::string& appid,
                                        const std::string& statusForComparison)
        {
            auto search = m_statusCache.find(appid);
            if (search == m_statusCache.end())
            {
                return updateStatus(appid, statusForComparison);
            }
            if (search->second == statusForComparison)
            {
                return false;
            }
            return updateStatus(appid, statusForComparison);
        }

        //TODO: LINUXEP-6339 	Implement a Persistent Status Cache for Management Agent
        bool StatusCache::updateStatus(const std::string& appid,
                                       const std::string& statusForComparison)
        {
            m_statusCache[appid] = statusForComparison;
            return true;
        }

    }
}