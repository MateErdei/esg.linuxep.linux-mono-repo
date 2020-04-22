/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "IQueryRunner.h"

#include <Common/TelemetryHelperImpl/TelemetryObject.h>
namespace queryrunner
{
    class Telemetry
    {
    public:
        void processLiveQueryResponseStats(const queryrunner::QueryRunnerStatus& response);
    };
} // namespace livequeryimpl
