/******************************************************************************************************

Copyright 2019-2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ResponseMetaData.h"
namespace livequery
{
    ResponseMetaData::ResponseMetaData()
    {
    }

    ResponseMetaData::ResponseMetaData(long queryStart)
    {
        m_queryStartMillisEpoch = queryStart;
    }

    long ResponseMetaData::getQueryStart() const
    {
        return m_queryStartMillisEpoch;
    }

}