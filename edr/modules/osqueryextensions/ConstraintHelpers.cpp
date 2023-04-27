// Copyright 2021-2023 Sophos Limited. All rights reserved.
#include "ConstraintHelpers.h"

#include "Logger.h"
#include "ThreatTypes.h"

#include <algorithm>

namespace OsquerySDK
{
    ConstraintHelpers::ConstraintHelpers()
        : m_startTime(0)
        , m_endTime(0)
    {
    }

    std::pair<uint64_t, uint64_t > ConstraintHelpers::GetTimeConstraints(QueryContextInterface& queryContext)
    {
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::EQUALS);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::GREATER_THAN);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::LESS_THAN);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::GREATER_THAN_OR_EQUALS);
        extractAndStoreTimeConstraint(queryContext, OsquerySDK::LESS_THAN_OR_EQUALS);

        obtainAndStoreStartAndEndTimes();

        return std::make_pair(m_startTime, m_endTime);
    }

    void ConstraintHelpers::extractAndStoreTimeConstraint(
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

    void ConstraintHelpers::obtainAndStoreStartAndEndTimes()
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

    uint64_t ConstraintHelpers::obtainStartBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime)
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

    uint64_t ConstraintHelpers::obtainEndBoundaryTime(TimeConstraintData& timeData, uint64_t compareTime)
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
    std::string ConstraintHelpers::GetOperatorSymbol(const unsigned char osqueryOperator)
    {
        switch (osqueryOperator)
        {
            case 2:
                return "=";
            case 4:
                return ">";
            case 8:
                return "<=";
            case 16:
                return "<";
            case 32:
                return ">=";
            case 64:
                return "MATCH";
            case 65:
                return "LIKE";
            case 66:
                return "GLOB";
            case 67:
                return "REGEXP";
            case 68:
                // Could also be <>
                return "!=";
            case 73:
                return "LIMIT";
            case 74:
                return "OFFSET";
            case 1:
                return "UNIQUE";
            default:
                LOGERROR("Unknown query operator: " + std::to_string(osqueryOperator));
                return "Unknown operator(" + std::to_string(osqueryOperator) + ")";
        }
    }
    std::string ConstraintHelpers::BuildQueryString(OsquerySDK::QueryContextInterface& context, const std::string& tableName)
    {
        std::string columnsString;
        for (auto const& column : context.GetUsedColumns())
        {
            columnsString += (column + ", ");
        }
        columnsString = columnsString.substr(0, columnsString.size() - 2);

        if (context.CountAllConstraints() == 0)
        {
            return "SELECT " + columnsString + " FROM " + tableName;
        }

        std::string constraintsString;
        std::map<OsquerySDK::ConstraintOperator, std::string> suffixConstraints;
        for (auto const& element : context.GetAllConstraints())
        {
            if (element.second.affinity == OsquerySDK::ColumnType::TEXT_TYPE)
            {
                for (auto constraint : element.second.constraints)
                {
                    if (OsquerySDK::IGNORED_CONSTRAINTS.find(constraint.op) != OsquerySDK::IGNORED_CONSTRAINTS.end())
                    {
                        suffixConstraints[constraint.op] = constraint.expr;
                    }
                    else
                    {
                        constraintsString +=
                            (element.first + " " + GetOperatorSymbol(constraint.op) + " '" + constraint.expr +
                             "' AND ");
                    }
                }
            }
            else
            {
                for (auto constraint : element.second.constraints)
                {
                    constraintsString +=
                        (element.first + " " + GetOperatorSymbol(constraint.op) + " " + constraint.expr + " AND ");
                }
            }
        }
        constraintsString = constraintsString.substr(0, constraintsString.size() - 5);

        std::string suffixString = "";
        for (auto constraint : suffixConstraints)
        {
            suffixString += " " + GetOperatorSymbol(constraint.first) + " " + constraint.second;
        }

        if (!constraintsString.empty())
        {
            constraintsString = " WHERE " + constraintsString;
        }

        return "SELECT " + columnsString + " FROM " + tableName + constraintsString + suffixString;
    }

    std::string ConstraintHelpers::BuildQueryLogMessage(OsquerySDK::QueryContextInterface& context, const std::string& tableName)
    {
        try
        {
            if (context.CountAllConstraints() == 0)
            {
                return tableName + " queried with no constraints: " + BuildQueryString(context, tableName);
            }

            return tableName + " queried with the following: " + BuildQueryString(context, tableName);
        }
        catch (const std::exception& e)
        {
            LOGERROR(e.what());
            return tableName + " queried, failed to build query string";
        }
    }
}