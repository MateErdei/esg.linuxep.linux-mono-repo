// Copyright 2019-2023 Sophos Limited. All rights reserved.

#include "IQueryProcessor.h"
#include "Logger.h"
#include "ResponseDispatcher.h"

// Auto version headers
#include "AutoVersioningHeaders/AutoVersion.h"

#include "Common/TelemetryHelperImpl/TelemetryJsonToMap.h"

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

    class ScopedFeedbackProvider
    {
        livequery::IResponseDispatcher& m_dispatcher;
        livequery::IResponseDispatcher::QueryResponseStatus m_status;

    public:
        ScopedFeedbackProvider(livequery::IResponseDispatcher& dispatcher) :
            m_dispatcher(dispatcher),
            m_status { livequery::IResponseDispatcher::QueryResponseStatus::UnexpectedExceptionOnHandlingQuery }
        {
        }
        void setFeedback(livequery::IResponseDispatcher::QueryResponseStatus queryResponseStatus)
        {
            m_status = queryResponseStatus;
        }

        ~ScopedFeedbackProvider()
        {
            m_dispatcher.feedbackResponseStatus(m_status);
        }
    };

} // namespace

int livequery::processQuery(
    livequery::IQueryProcessor& iQueryProcessor,
    livequery::IResponseDispatcher& dispatcher,
    const std::string& queryJson,
    const std::string& correlationId)
{
    LOGINFO("LiveQuery " << _AUTOVER_COMPONENTAUTOVERSION_STR_ << " started");
    ScopedFeedbackProvider scopedFeedbackProvider { dispatcher };
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
        scopedFeedbackProvider.setFeedback(livequery::IResponseDispatcher::QueryResponseStatus::QueryInvalidJson);
        return INVALIDJSONERROR;
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
        scopedFeedbackProvider.setFeedback(livequery::IResponseDispatcher::QueryResponseStatus::QueryFailedValidation);
        return INVALIDREQUESTERROR;
    }

    // check option value is string
    if (queryIter->second.getType() != Common::Telemetry::TelemetryValue::Type::string_type ||
        queryNameIter->second.getType() != Common::Telemetry::TelemetryValue::Type::string_type)
    {
        LOGWARN("Invalid request, required json value 'query' must be a string");
        LOGDEBUG("Content of input request: " << queryJson);
        scopedFeedbackProvider.setFeedback(livequery::IResponseDispatcher::QueryResponseStatus::QueryFailedValidation);
        return INVALIDQUERYERROR;
    }

    try
    {
        std::string queryDetails =
            "name: " + queryNameIter->second.getString() + " and corresponding id: " + correlationId;
        LOGINFO("Executing query " << queryDetails);

        std::string query = queryIter->second.getString();
        LOGDEBUG(query);

        livequery::QueryResponse response = iQueryProcessor.query(query);

        // Set name of query in metadata.
        ResponseMetaData responseMetaData = response.metaData();
        responseMetaData.setQueryName(queryNameIter->second.getString());
        response.setMetaData(responseMetaData);

        if (response.status().errorCode() == livequery::ErrorCode::SUCCESS)
        {
            LOGINFO("Successfully executed query with " << queryDetails);
        }
        else
        {
            LOGINFO(
                "Query with " << queryDetails
                              << " failed to execute with error: " << response.status().errorDescription());
        }

        dispatcher.sendResponse(correlationId, response);
        scopedFeedbackProvider.setFeedback(livequery::IResponseDispatcher::QueryResponseStatus::QueryResponseProduced);
    }
    catch (const std::exception& ex)
    {
        LOGWARN("Error while executing query");
        LOGSUPPORT("Error information: " << ex.what());
        LOGDEBUG("Content of input request: '" << queryIter->second.getString() << "'");
        scopedFeedbackProvider.setFeedback(
            livequery::IResponseDispatcher::QueryResponseStatus::UnexpectedExceptionOnHandlingQuery);
        return FAILEDTOEXECUTEERROR;
    }
    return 0;
}