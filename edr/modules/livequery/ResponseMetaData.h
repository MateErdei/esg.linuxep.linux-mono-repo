/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>

namespace livequery
{
    class ResponseMetaData
    {
    public:
        ResponseMetaData();
        ResponseMetaData(long queryStart);
        long getQueryStart() const;

    private:
        long m_queryStartMillisEpoch = 0;
    };
} // namespace livequery
