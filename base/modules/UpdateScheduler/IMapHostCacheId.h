/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace UpdateScheduler
{
    class IMapHostCacheId
    {
    public:
        virtual ~IMapHostCacheId() = default;
        /**
         * Must return the Id that identifies the update Cache of hostname or empty if the url does not
         * correspond to any known update cache. The information is used for filling up updateSource as
         *   https://wiki.sophos.net/display/SophosCloud/EMP%3A+event-alc
         * @param updateCacheUrl
         * @return : Id of Update Cache or empty string
         */
        virtual std::string cacheID(const std::string& updateCacheUrl) const = 0;
    };
} // namespace UpdateScheduler