/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "Telemetry.h"

#include <Common/TelemetryHelperImpl/TelemetryHelper.h>
#include <Common/UtilityImpl/StringUtils.h>

void livequery::Telemetry::processLiveQueryResponseStats(const livequery::QueryResponse& response, long queryDuration)
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();

    std::string strippedQueryName =
        Common::UtilityImpl::StringUtils::replaceAll(response.metaData().getQueryName(), ".", "-");
    std::string livequeryKey = "live-query." + strippedQueryName;

    switch (response.status().errorCode())
    {
        case livequery::ErrorCode::SUCCESS:
            telemetry.increment(livequeryKey + ".successful-count", 1L);
            telemetry.appendStat(livequeryKey + ".duration", queryDuration);
            telemetry.appendStat(livequeryKey + ".rowcount", response.data().columnData().size());
            break;
        case livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING:
            telemetry.increment(livequeryKey + ".failed-osquery-died-count", 1L);
            break;
        case livequery::ErrorCode::OSQUERYERROR:
            telemetry.increment(livequeryKey + ".failed-osquery-error-count", 1L);
            break;
        case livequery::ErrorCode::RESPONSEEXCEEDLIMIT:
            telemetry.increment(livequeryKey + ".failed-exceed-limit-count", 1L);
            break;
        case livequery::ErrorCode::UNEXPECTEDERROR:
            telemetry.increment(livequeryKey + ".failed-unexpected-error-count", 1L);
            break;
    }
}
