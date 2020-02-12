/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ResponseMetaData.h"

#include <chrono>

namespace livequery
{
    ResponseMetaData::ResponseMetaData(long queryStart)
    {
        m_queryStartMillisEpoch = queryStart;
    }

    long ResponseMetaData::getQueryStart() const
    {
        return m_queryStartMillisEpoch;
    }
    long ResponseMetaData::getMillisSinceStarted() const
    {
        auto nowMilliEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count();
        return nowMilliEpoch - getQueryStart();
    }
}