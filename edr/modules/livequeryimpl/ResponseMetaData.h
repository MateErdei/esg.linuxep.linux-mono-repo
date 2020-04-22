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
        ResponseMetaData() = default;
        ResponseMetaData(long queryStart);
        long getQueryStart() const;
        long getMillisSinceStarted() const;
        void setQueryName(const std::string& queryName);
        std::string getQueryName() const;
    private:
        long m_queryStartMillisEpoch = 0;
        std::string m_queryName;
    };
} // namespace livequeryimpl
