/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueryResponse.h"

#include <Common/TelemetryHelperImpl/TelemetryObject.h>
namespace livequery
{
    class Telemetry
    {
    public:
        void processLiveQueryResponseStats(const livequery::QueryResponse& response, long queryDuration);
    };
} // namespace livequeryimpl
