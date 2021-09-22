/******************************************************************************************************

Copyright 2021 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "TimeConstraintHelpers.h"

#include "ThreatTypes.h"

#include <algorithm>

namespace OsquerySDK
{
    TimeConstraintHelpers::TimeConstraintHelpers()
        : m_startTime(0)
        , m_endTime(0)
    {
    }

    std::pair<uint64_t, uint64_t > TimeConstraintHelpers::GetTimeConstraints(QueryContextInterface& queryContext)
    {
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::EQUALS);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::GREATER_THAN);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::LESS_THAN);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::GREATER_THAN_OR_EQUALS);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::LESS_THAN_OR_EQUALS);

        obtainAndStoreStartAndEndTimes();

        return std::make_pair(m_startTime, m_endTime);
    }

    void TimeConstraintHelpers::extractAndStoreTimeConstraint(
        QueryContextInterface& queryContext,
        OsquerySDK::ConstraintOperator constraintOperator)
    {
        std::set<std::string> constraints = queryContext.GetConstraints("time", constraintOperator);

        for (auto& constraint : constraints)
        {
            TimeConstraintData constraintData;
            try
            {
                constraintData.time = std::stoll(constraint);
            }
            catch(...)
            {
                constraintData.time = 0; // set to default 'not-set' value for invalid data constraint value
            }

            constraintData.constraintType = constraintOperator;

            if (constraintOperator == OsquerySDK::EQUALS)
            {
                m_timeEqualsConstraints.push_back(constraintData);
            }
            else
            {
                m_timeBoundaryConstraints.push_back(constraintData);
            }
        }
    }

    void TimeConstraintHelpers::obtainAndStoreStartAndEndTimes()
    {
        // first sort both constraint lists
        auto compare = [](const  TimeConstraintData &a, const TimeConstraintData &b) {
          return a.time < b.time;};

        std::sort(std::begin(m_timeEqualsConstraints), std::end(m_timeEqualsConstraints), compare);
        std::sort(std::begin(m_timeBoundaryConstraints), std::end(m_timeBoundaryConstraints), compare);

        if (!m_timeEqualsConstraints.empty())
        {
            m_startTime = m_timeEqualsConstraints.front().time;
            m_endTime = m_timeEqualsConstraints.back().time;
        }

        if (!m_timeBoundaryConstraints.empty())
        {
            m_startTime = obtainStartBoundaryTime(m_timeBoundaryConstraints.front(), m_startTime);
            m_endTime = obtainEndBoundaryTime(m_timeBoundaryConstraints.back(), m_endTime);
        }

        if (m_timeEqualsConstraints.empty() && m_timeBoundaryConstraints.empty())
        {
            auto now = std::chrono::system_clock::now();
            m_startTime = std::chrono::system_clock::to_time_t(now-std::chrono::minutes(DEFAULT_START_TIME_OFFSET_MINUTES));
        }
    }

    uint64_t TimeConstraintHelpers::obtainStartBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime)
    {

        uint64_t startTime = 0;
        if (timeData.constraintType == OsquerySDK::GREATER_THAN
            || timeData.constraintType == OsquerySDK::GREATER_THAN_OR_EQUALS )
        {
            startTime = timeData.time;
        }
        else  if (timeData.constraintType == OsquerySDK::LESS_THAN
                  || timeData.constraintType == OsquerySDK::LESS_THAN_OR_EQUALS)
        {
            return 0; // means un-set
        }

        if(compareTime != 0 && startTime != 0)
        {
            return std::min(startTime, compareTime);
        }
        else
        {
            return std::max(startTime, compareTime);
        }
    }

    uint64_t TimeConstraintHelpers::obtainEndBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime)
    {
        uint64_t endTime = 0;
        if (timeData.constraintType == OsquerySDK::LESS_THAN
            || timeData.constraintType == OsquerySDK::LESS_THAN_OR_EQUALS)
        {
            endTime = timeData.time;
        }
        else if (timeData.constraintType == OsquerySDK::GREATER_THAN
                 || timeData.constraintType == OsquerySDK::GREATER_THAN_OR_EQUALS )
        {
            return 0; // used to mean un-set
        }
        return std::max(endTime, compareTime);

    }
}