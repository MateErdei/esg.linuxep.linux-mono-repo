/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma  once

#include <string>
#include "ResponseData.h"
#include "ResponseStatus.h"
#include "ResponseMetaData.h"
//ResponseStatus

//        ResponseData

//ResponseMetaData
namespace livequery{
    class QueryResponse {
        QueryResponse();
    public:
        static QueryResponse emptyResponse()
        {
            return QueryResponse{};
        }
        QueryResponse( ResponseStatus status, ResponseData data):
           m_status(std::move(status)),
           m_data( std::move(data))
        {

        }
        void setMetaData(const ResponseMetaData& );
        const ResponseStatus & status() const;
        const ResponseMetaData & metaData() const;
        const ResponseData & data() const;

    private:
        ResponseStatus m_status;
        ResponseMetaData m_metaData;
        ResponseData m_data;
    };
}

