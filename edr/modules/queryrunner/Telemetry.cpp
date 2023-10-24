// Copyright 2020-2023 Sophos Limited. All rights reserved.
#include "Telemetry.h"

#include "Common/TelemetryHelperImpl/TelemetryHelper.h"

#include "EdrCommon/TelemetryConsts.h"

namespace queryrunner{
void Telemetry::processLiveQueryResponseStats(const queryrunner::QueryRunnerStatus& response)
{
    auto& telemetry = Common::Telemetry::TelemetryHelper::getInstance();


    std::string livequeryKey = std::string(plugin::telemetryLiveQueries) + ".";

    switch (response.errorCode)
    {
        case livequery::ErrorCode::SUCCESS:
            telemetry.increment(livequeryKey  + plugin::telemetrySuccessfulQueries, 1L);
            break;
        case livequery::ErrorCode::EXTENSIONEXITEDWHILERUNNING:
            telemetry.increment(livequeryKey +  plugin::telemetryFailedQueries, 1L);
            break;
        case livequery::ErrorCode::OSQUERYERROR:
            telemetry.increment(livequeryKey + plugin::telemetryFailedQueries, 1L);
            break;
        case livequery::ErrorCode::RESPONSEEXCEEDLIMIT:
            telemetry.increment(livequeryKey +  plugin::telemetryFailedQueries, 1L);
            break;
        case livequery::ErrorCode::UNEXPECTEDERROR:
            telemetry.increment(livequeryKey +  plugin::telemetryFailedQueries, 1L);
            break;
    }
}
}
