// Copyright 2020-2023 Sophos Limited. All rights reserved.
#pragma once
#include "IQueryRunner.h"

#include "Common/TelemetryHelperImpl/TelemetryObject.h"

namespace queryrunner
{
    class Telemetry
    {
    public:
        void processLiveQueryResponseStats(const queryrunner::QueryRunnerStatus& response);
    };
} // namespace livequery
