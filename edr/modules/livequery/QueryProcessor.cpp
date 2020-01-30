/******************************************************************************************************

Copyright 2019-2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IQueryProcessor.h"
#include "Logger.h"
#include "ResponseDispatcher.h"

#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>

/**
 * Given the queryJson it will:
 *   1) Validate the json conforming to:
 * https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+action-run-live-query 2) Validate
 * that the 3 required fields are available: type, name, query 3) Extract query and give to iQueryProcessor. 4) Pass the
 * information returned from the iQueryProcessor to ResponseDispatcher that will create the json and move it to the
 * correct place.
 *
 * @param iQueryProcessor
 * @param queryJson
 * @param correlationId
 */

namespace
{
    const char* queryTypeKey = "type";
    const char* queryNameKey = "name";
    const char* querySqlKey = "query";
} // namespace

void livequery::processQuery(
    livequery::IQueryProcessor& iQueryProcessor,
    livequery::IResponseDispatcher& dispatcher,
    const std::string& queryJson,
    const std::string& correlationId)
{
    std::unordered_map<std::string, Common::Telemetry::TelemetryValue> requestMap;
    try
    {
        requestMap = Common::Telemetry::flatJsonToMap(queryJson);
    }
    catch (std::runtime_error& ex)
    {
        LOGWARN("Received an invalid request, failed to parse the json input.");
        LOGSUPPORT("Invalid request, failed to parse the json with error: " << ex.what());
        LOGDEBUG("Content of input request: " << queryJson);
        return;
    }

    auto queryIter = requestMap.find(querySqlKey);
    auto queryNameIter = requestMap.find(queryNameKey);
    if (requestMap.find(queryTypeKey) == requestMap.end() || queryNameIter == requestMap.end() ||
        queryIter == requestMap.end())
    {
        // log invalid query abort
        LOGWARN(
            "Invalid request, required json values are: '" << queryTypeKey << "', '" << queryNameKey << "' and '"
                                                           << querySqlKey);
        LOGDEBUG("Content of input request: " << queryJson);
        return;
    }

    // check option value is string
    if (queryIter->second.getType() != Common::Telemetry::TelemetryValue::Type::string_type ||
        queryNameIter->second.getType() != Common::Telemetry::TelemetryValue::Type::string_type)
    {
        LOGWARN("Invalid request, required json value 'query' must be a string");
        LOGDEBUG("Content of input request: " << queryJson);
        return;
    }

    try
    {
        LOGINFO(
            "Executing query name: " << queryNameIter->second.getString()
                                     << " and corresponding id: " << correlationId);
        std::string query = queryIter->second.getString();
        LOGDEBUG(query);
        livequery::QueryResponse response = iQueryProcessor.query(query);
        dispatcher.sendResponse(correlationId, response);
        LOGINFO("Finished executing query with name: " << queryNameIter->second.getString()
                                         << " and corresponding id: " << correlationId);
    }
    catch (const std::exception& ex)
    {
        LOGWARN("Error while executing query");
        LOGSUPPORT("Error information: " << ex.what());
        LOGDEBUG("Content of input request: '" << queryIter->second.getString() << "'");
    }
}