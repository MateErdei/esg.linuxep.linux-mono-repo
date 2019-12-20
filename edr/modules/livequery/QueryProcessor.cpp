//
// Created by pair on 19/12/2019.
//

#include "IQueryProcessor.h"
#include "ResponseDispatcher.h"
#include <Common/TelemetryHelperImpl/TelemetryJsonToMap.h>
#include "Logger.h"

/**
 * Given the queryJson it will:
 *   1) Validate the json conforming to: https://wiki.sophos.net/pages/viewpage.action?spaceKey=SophosCloud&title=EMP%3A+action-run-live-query
 *   2) Validate that the 3 required fields are available: type, name, query
 *   3) Extract query and give to iQueryProcessor.
 *   4) Pass the information returned from the iQueryProcessor to ResponseDispatcher that will create the json and move it to the correct place.
 *
 * @param iQueryProcessor
 * @param queryJson
 * @param correlationId
 */

namespace
{
    const char * queryTypeKey = "type";
    const char * queryNameKey = "name";
    const char * querySqlKey = "query";
}

void livequery::processQuery(livequery::IQueryProcessor &iQueryProcessor, livequery::IResponseDispatcher& dispatcher,
        const std::string &queryJson, const std::string &correlationId)
{
    auto queryMap = Common::Telemetry::flatJsonToMap(queryJson);
    auto queryIter = queryMap.find(querySqlKey);
    if ( queryMap.find(queryTypeKey) == queryMap.end() || queryMap.find(queryNameKey) == queryMap.end() || queryIter == queryMap.end())
    {
        //log invalid query abort
        LOGWARN("Invalid query, required json values are: '" << queryTypeKey <<"', '" << queryNameKey << "'and '"<< querySqlKey);
        return;
    }

    //check option value is string
    std::string query = queryIter->second.getString();
    livequery::QueryResponse response =  iQueryProcessor.query(query);
    dispatcher.sendResponse(correlationId, response);

}