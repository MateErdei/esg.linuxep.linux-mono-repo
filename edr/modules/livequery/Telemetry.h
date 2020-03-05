/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueryResponse.h"
#include "QueryTelemetryStats.h"

#include <Common/TelemetryHelperImpl/TelemetryObject.h>
namespace livequery
{
    class Telemetry
    {
    public:
        void processLiveQueryResponseStats(const livequery::QueryResponse& response, long queryDuration);

    private:
        std::map<std::string, QueryTelemetryStats> m_queryStats;
    };
} // namespace livequery
