/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "QueryTelemetryStats.h"

#include <algorithm>

namespace livequery
{
    QueryTelemetryStats::QueryTelemetryStats() :
        m_failedCountOsqueryDied(0),
        m_failedCountOsqueryError(0),
        m_failedCountLimitExceeded(0),
        m_failedCountUnexpected(0),
        m_successfulCount(0),
        m_successfulDurations({}),
        m_successfulRowCounts({})
    {
    }

    unsigned long QueryTelemetryStats::getSuccessfulCount() const
    {
        return m_successfulCount;
    }

    void QueryTelemetryStats::incrementSuccessfulCount()
    {
        m_successfulCount++;
    }

    unsigned long QueryTelemetryStats::getFailedCountOsqueryDied() const
    {
        return m_failedCountOsqueryDied;
    }

    void QueryTelemetryStats::incrementFailedCountOsqueryDied()
    {
        m_failedCountOsqueryDied++;
    }

    unsigned long QueryTelemetryStats::getFailedCountOsqueryError() const
    {
        return m_failedCountOsqueryError;
    }

    void QueryTelemetryStats::incrementFailedCountOsqueryError()
    {
        m_failedCountOsqueryError++;
    }

    unsigned long QueryTelemetryStats::getFailedCountLimitExceeded() const
    {
        return m_failedCountLimitExceeded;
    }

    void QueryTelemetryStats::incrementFailedCountLimitExceeded()
    {
        m_failedCountLimitExceeded++;
    }

    unsigned long QueryTelemetryStats::getFailedCountUnexpected() const
    {
        return m_failedCountUnexpected;
    }

    void QueryTelemetryStats::incrementFailedCountUnexpected()
    {
        m_failedCountUnexpected++;
    }

    void QueryTelemetryStats::addSuccessfulDuration(unsigned long duration)
    {
        m_successfulDurations.push_back(duration);
    }

    void QueryTelemetryStats::addSuccessfulRowCount(unsigned long rowCount)
    {
        m_successfulRowCounts.push_back(rowCount);
    }

    unsigned long QueryTelemetryStats::getSuccessfulAverageDuraiton() const
    {
        unsigned long summation = 0;

        for (auto& duration : m_successfulDurations)
        {
            summation += duration;
        }

        return summation / m_successfulDurations.size();
    }

    unsigned long QueryTelemetryStats::getSuccessfulMinDuraiton() const
    {
        return *std::min_element(m_successfulDurations.begin(), m_successfulDurations.end());
    }

    unsigned long QueryTelemetryStats::getSuccessfulMaxDuraiton() const
    {
        return *std::max_element(m_successfulDurations.begin(), m_successfulDurations.end());
    }

    unsigned long QueryTelemetryStats::getSuccessfulAverageRowCount() const
    {
        unsigned long summation = 0;

        for (auto& rowCount : m_successfulRowCounts)
        {
            summation += rowCount;
        }

        return summation / m_successfulRowCounts.size();
    }

}