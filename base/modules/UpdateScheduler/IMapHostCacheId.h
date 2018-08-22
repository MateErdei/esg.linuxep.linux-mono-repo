/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>


namespace UpdateSchedulerImpl
{
    class IMapHostCacheId
    {
    public:
        virtual  ~IMapHostCacheId() = default;
        virtual std::string cacheID(const std::string & hostname) const = 0;
    };
}