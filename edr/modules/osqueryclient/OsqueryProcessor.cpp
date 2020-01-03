/******************************************************************************************************

Copyright 2019 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryProcessor.h"
#include <livequery/ResponseData.h>
#include <livequery/ResponseStatus.h>
#include <extensions/interface.h>
#include <osquery/flags.h>
#include <osquery/flagalias.h>
#include <iostream>
namespace osquery{

    FLAG(bool, decorations_top_level, false, "test");
    std::unique_ptr<osquery::ExtensionManagerAPI> makeClient(std::string socket)
    {
        return std::make_unique<osquery::ExtensionManagerClient>(socket, 10000);
    }
}


namespace osqueryclient
{

    OsqueryProcessor::OsqueryProcessor(const std::string &socketPath) : m_socketPath(socketPath) {

    }


    livequery::QueryResponse OsqueryProcessor::query(const std::string & query)
    {
        osquery::QueryData qd;
        auto client = osquery::makeClient(m_socketPath);
        auto osqueryStatus = client->query(query, qd);
        if ( !osqueryStatus.ok())
        {
            livequery::ResponseStatus responseStatus{ livequery::ErrorCode::OSQUERYERROR};
            responseStatus.overrideErrorDescription(osqueryStatus.getMessage());
            return livequery::QueryResponse{ responseStatus, livequery::ResponseData::emptyResponse()};
        }
        osquery::QueryData qcolumn;
        osqueryStatus = client->getQueryColumns(query, qcolumn);
        if ( !osqueryStatus.ok())
        {
            livequery::ResponseStatus responseStatus{ livequery::ErrorCode::OSQUERYERROR};
            responseStatus.overrideErrorDescription(osqueryStatus.getMessage());
            return livequery::QueryResponse{ responseStatus, livequery::ResponseData::emptyResponse()};
        }
        livequery::ResponseData::ColumnData  columnData = qd;
        livequery::ResponseData::ColumnHeaders headers;

        for (const auto& row : qcolumn)
        {
            const std::string & columnName = row.begin()->first;
            const std::string & columnType = row.begin()->second;
            if (columnType == "BIGINT" || columnType == "INTEGER" || columnType == "UNSIGNED BIGINT")
            {
                headers.emplace_back(columnName, livequery::ResponseData::AcceptedTypes::BIGINT);
            }
            else
            {
                headers.emplace_back(columnName, livequery::ResponseData::AcceptedTypes::STRING);
            }
        }

        livequery::ResponseStatus status{ livequery::ErrorCode::SUCCESS};

        livequery::QueryResponse response{status, livequery::ResponseData{headers,columnData}};

        livequery::ResponseMetaData metaData;
        response.setMetaData(metaData);

        return response;
    }

}
