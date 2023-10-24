// Copyright 2020-2023 Sophos Limited. All rights reserved.

#include "IOsqueryClient.h"
#include "Logger.h"
#include "OsqueryProcessor.h"

#include "livequery/ResponseData.h"
#include "queryrunner/ResponseStatus.h"

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
     * Osquery getQueryColumns will return only the information of name for the queries above.
     *
     * For this reason, this method starts from the information given in the queryColumnsInfo and deduce any extra
     * column based on the data provided by osquery.
     * */
    livequery::ResponseData::ColumnHeaders updateColumnDataFromValues(
        OsquerySDK::QueryColumns& queryColumnsInfo,
        const OsquerySDK::QueryData& queryData)
    {
        livequery::ResponseData::ColumnHeaders headers;
        std::unordered_set<std::string> columnsReported;
        for (const auto& row : queryColumnsInfo)
        {
            headers.emplace_back(row.name, row.type);
            columnsReported.insert(row.name);
        }

        for (auto& row : queryData)
        {
            for (auto& keyValue : row)
            {
                const std::string& columnName = keyValue.first;
                if (columnsReported.find(columnName) == columnsReported.end())
                {
                    columnsReported.insert(columnName);
                    headers.emplace_back(columnName, OsquerySDK::ColumnType::TEXT_TYPE);
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

        return livequery::QueryResponse {status, livequery::ResponseData::emptyResponse(), livequery::ResponseMetaData()};
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
            OsquerySDK::QueryData queryData;
            auto client = osqueryclient::factory().create();
            client->connect(m_socketPath);

            // Metadata includes query duration
            long start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

            auto osqueryStatus = client->query(query, queryData);
            if (osqueryStatus.code != 0)
            {
                return failureQueryResponse(livequery::ErrorCode::OSQUERYERROR, osqueryStatus.message);
            }

            OsquerySDK::QueryColumns queryColumnsInfo;

            osqueryStatus = client->getQueryColumns(query, queryColumnsInfo);
            if (osqueryStatus.code != 0)
            {
                return failureQueryResponse(livequery::ErrorCode::OSQUERYERROR, osqueryStatus.message);
            }

            livequery::ResponseData::ColumnHeaders headers = updateColumnDataFromValues(queryColumnsInfo, queryData);

            livequery::ResponseStatus status { livequery::ErrorCode::SUCCESS };

            livequery::QueryResponse response { status, livequery::ResponseData { headers, queryData }, livequery::ResponseMetaData{start}};

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

    std::unique_ptr<livequery::IQueryProcessor> OsqueryProcessor::clone() {
        return std::unique_ptr<livequery::IQueryProcessor>(new OsqueryProcessor(m_socketPath));
    }

} // namespace osqueryclient
