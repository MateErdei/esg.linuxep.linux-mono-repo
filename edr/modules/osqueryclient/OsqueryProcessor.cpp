/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "OsqueryProcessor.h"

#include "IOsqueryClient.h"
#include "Logger.h"

#include <livequery/ResponseData.h>
#include <livequery/ResponseStatus.h>
#include <thrift/transport/TTransportException.h>

#include <iostream>
#include <unordered_set>

namespace
{
    /*There are many use cases where osquery is not able to provide the columns for the query to be executed.
     * For example:
     * CREATE TABLE hello(bar int)
     * select * from hello
     * Because the queryColumns does not 'create the table' it is not able to deduce the headers and will return
     * empty.
     * For tests, the easiest way to 'reproduce' this issue is to issue 2 queries. For example:
     * select name from processes; select pid from processes;
     * Osquery getQueryColumns will return only the informatio of name for the queries above.
     *
     * For this reason, this method starts from the information given in the queryColumnsInfo and deduce any extra
     * column based on the data provided by osquery.
     * */
    livequery::ResponseData::ColumnHeaders updateColumnDataFromValues(
        osquery::QueryData& queryColumnsInfo,
        const osquery::QueryData& queryData)
    {
        livequery::ResponseData::ColumnHeaders headers;
        std::unordered_set<std::string> columnsReported;
        for (const auto& row : queryColumnsInfo)
        {
            const std::string& columnName = row.begin()->first;
            const std::string& columnTypeStr = row.begin()->second;
            livequery::ResponseData::AcceptedTypes columnType =
                livequery::ResponseData::AcceptedTypesFromString(columnTypeStr);
            headers.emplace_back(columnName, columnType);
            columnsReported.insert(columnName);
        }

        for (auto& row : queryData)
        {
            for (auto& keyValue : row)
            {
                const std::string& columnName = keyValue.first;
                if (columnsReported.find(columnName) == columnsReported.end())
                {
                    columnsReported.insert(columnName);
                    headers.emplace_back(columnName, livequery::ResponseData::AcceptedTypes::STRING);
                }
            }
        }
        return headers;
    }

    livequery::QueryResponse failureQueryResponse(
        livequery::ErrorCode errorCode,
        const std::string& overrideMessage = "")
    {
        livequery::ResponseStatus status { errorCode };
        if (!overrideMessage.empty())
        {
            status.overrideErrorDescription(overrideMessage);
        }

        return livequery::QueryResponse { status, livequery::ResponseData::emptyResponse() };
    }
} // namespace

namespace osqueryclient
{
    OsqueryProcessor::OsqueryProcessor(std::string socketPath) : m_socketPath(std::move(socketPath))
    {
    }

    livequery::QueryResponse OsqueryProcessor::query(const std::string& query)
    {
        try
        {
            osquery::QueryData queryData;
            auto client = osqueryclient::factory().create();
            client->connect(m_socketPath);

            auto osqueryStatus = client->query(query, queryData);
            if (!osqueryStatus.ok())
            {
                return failureQueryResponse(livequery::ErrorCode::OSQUERYERROR, osqueryStatus.getMessage());
            }

            osquery::QueryData queryColumnsInfo;

            osqueryStatus = client->getQueryColumns(query, queryColumnsInfo);
            if (!osqueryStatus.ok())
            {
                return failureQueryResponse(livequery::ErrorCode::OSQUERYERROR, osqueryStatus.getMessage());
            }

            livequery::ResponseData::ColumnHeaders headers = updateColumnDataFromValues(queryColumnsInfo, queryData);

            livequery::ResponseStatus status { livequery::ErrorCode::SUCCESS };

            livequery::QueryResponse response { status, livequery::ResponseData { headers, queryData } };

            livequery::ResponseMetaData metaData;
            response.setMetaData(metaData);

            return response;
        }
        catch (osqueryclient::FailedToStablishConnectionException& ex)
        {
            LOGWARN("Failed to connect to osquery");
            return failureQueryResponse(livequery::ErrorCode::UNEXPECTEDERROR, ex.what());
        }
        catch (apache::thrift::transport::TTransportException& ex)
        {
            // this usually happens when osquery breaks in the middle of a query. Thrift does not 'automatically' detect
            // the problem. But it will timeout and generate an exception.
            LOGWARN("Livequery failed while processing query: " << ex.what());
            return failureQueryResponse(livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING);
        }
    }

} // namespace osqueryclient
