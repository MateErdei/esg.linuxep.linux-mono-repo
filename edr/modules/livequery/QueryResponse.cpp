/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueryResponse.h"
namespace livequery
{
    QueryResponse::QueryResponse() :
        m_status { ErrorCode::SUCCESS },
        m_metaData {},
        m_data { ResponseData::emptyResponse() }
    {
    }

    void QueryResponse::setMetaData(const ResponseMetaData& metaData)
    {
        m_metaData = metaData;
    }

    const ResponseStatus& QueryResponse::status() const
    {
        return m_status;
    }

    const ResponseMetaData& QueryResponse::metaData() const
    {
        return m_metaData;
    }

    const ResponseData& QueryResponse::data() const
    {
        return m_data;
    }
} // namespace livequery