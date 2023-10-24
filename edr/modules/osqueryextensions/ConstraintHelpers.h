// Copyright 2021-2023 Sophos Limited. All rights reserved.

#pragma once

#ifdef SPL_BAZEL
#include "common/livequery/include/OsquerySDK/OsquerySDK.h"
#else
#include "OsquerySDK/OsquerySDK.h"
#endif

namespace OsquerySDK
{
    struct TimeConstraintData{
        uint64_t time;
        OsquerySDK::ConstraintOperator constraintType;
    };

    class ConstraintHelpers
    {
    public:
        ConstraintHelpers();
        ~ConstraintHelpers() = default;
        std::pair<uint64_t, uint64_t >  GetTimeConstraints(QueryContextInterface& queryContext);

        std::string BuildQueryLogMessage(OsquerySDK::QueryContextInterface& context, const std::string& tableName);

    private:
        const uint32_t DEFAULT_START_TIME_OFFSET_MINUTES = 15;
        std::string GetOperatorSymbol(const unsigned char osqueryOperator);
        std::string BuildQueryString(OsquerySDK::QueryContextInterface& context, const std::string& tableName);
        void extractAndStoreTimeConstraint(
            QueryContextInterface& queryContext,
            OsquerySDK::ConstraintOperator constraintOperator);

        void obtainAndStoreStartAndEndTimes();
        uint64_t obtainStartBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime);
        uint64_t obtainEndBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime);

        // store for >, <, >= and  <= constraints
        std::vector<TimeConstraintData> m_timeBoundaryConstraints;
        // store for
        std::vector<TimeConstraintData> m_timeEqualsConstraints;

        uint64_t m_startTime;
        uint64_t m_endTime;
    };
}
