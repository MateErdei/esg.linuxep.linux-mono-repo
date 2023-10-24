// Copyright 2019-2023 Sophos Limited. All rights reserved.
#pragma once

#include "ResponseData.h"
#include "ResponseMetaData.h"

#include "queryrunner/ResponseStatus.h"

#include <string>

namespace livequery
{
    class QueryResponse
    {
        QueryResponse();

    public:
        static QueryResponse emptyResponse()
        {
            return QueryResponse {};
        }
        QueryResponse(ResponseStatus status, ResponseData data, ResponseMetaData metaData) : m_status(std::move(status)), m_metaData(metaData), m_data(std::move(data))
        {
        }
        void setMetaData(const ResponseMetaData&);
        const ResponseStatus& status() const;
        const ResponseMetaData& metaData() const;
        const ResponseData& data() const;

    private:
        ResponseStatus m_status;
        ResponseMetaData m_metaData;
        ResponseData m_data;
    };
} // namespace livequery
