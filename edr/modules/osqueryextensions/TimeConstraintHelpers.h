/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "OsquerySDK/OsquerySDK.h"

namespace OsquerySDK
{
    struct TimeConstraintData{
        uint64_t time;
        OsquerySDK::ConstraintOperator constraintType;
    };

    class TimeConstraintHelpers
    {
    public:
        TimeConstraintHelpers();
        ~TimeConstraintHelpers() = default;
        std::pair<uint64_t, uint64_t >  GetTimeConstraints(QueryContextInterface& queryContext);

    private:

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
