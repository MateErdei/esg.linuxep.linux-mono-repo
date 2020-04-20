/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Telemetry.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>
#include <modules/pluginimpl/TelemetryConsts.h>
namespace queryrunner{
void Telemetry::processLiveQueryResponseStats(const queryrunner::QueryRunnerStatus& response)
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

    std::string strippedQueryName =
        Common::UtilityImpl::StringUtils::replaceAll(response.name, ".", "-");
    std::string livequeryKey = "live-query." + strippedQueryName;

    switch (response.errorCode)
    {
        case livequery::ErrorCode::SUCCESS:
            telemetry.increment(livequeryKey + "." + plugin::telemetrySuccessfulQueries, 1L);
            telemetry.appendStat(livequeryKey + ".duration", response.queryDuration);
            telemetry.appendStat(livequeryKey + ".rowcount", response.rowCount);
            break;
        case livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING:
            telemetry.increment(livequeryKey + "." + plugin::telemetryFailedQueriesOsqueryDied, 1L);
            break;
        case livequery::ErrorCode::OSQUERYERROR:
            telemetry.increment(livequeryKey + "." + plugin::telemetryFailedQueriesOsqueryError, 1L);
            break;
        case livequery::ErrorCode::RESPONSEEXCEEDLIMIT:
            telemetry.increment(livequeryKey + "." + plugin::telemetryFailedQueriesLimitExceeded, 1L);
            break;
        case livequery::ErrorCode::UNEXPECTEDERROR:
            telemetry.increment(livequeryKey + "." + plugin::telemetryFailedQueriesUnexpected, 1L);
            break;
    }
}
}
