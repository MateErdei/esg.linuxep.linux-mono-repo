/******************************************************************************************************

Copyright 2020 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <vector>
namespace livequery
{
    class QueryTelemetryStats
    {
    public:
        QueryTelemetryStats();
        ~QueryTelemetryStats() = default;

        unsigned long getSuccessfulCount() const;
        void incrementSuccessfulCount();

        unsigned long getFailedCountOsqueryDied() const;
        void incrementFailedCountOsqueryDied();

        unsigned long getFailedCountOsqueryError() const;
        void incrementFailedCountOsqueryError();

        unsigned long getFailedCountLimitExceeded() const;
        void incrementFailedCountLimitExceeded();

        unsigned long getFailedCountUnexpected() const;
        void incrementFailedCountUnexpected();

        void addSuccessfulDuration(unsigned long duration);
        unsigned long getSuccessfulAverageDuraiton() const;
        unsigned long getSuccessfulMinDuraiton() const;
        unsigned long getSuccessfulMaxDuraiton() const;

        void addSuccessfulRowCount(unsigned long rowCount);
        unsigned long getSuccessfulAverageRowCount() const;

    private:
        unsigned int m_failedCountOsqueryDied;
        unsigned int m_failedCountOsqueryError;
        unsigned int m_failedCountLimitExceeded;
        unsigned int m_failedCountUnexpected;
        unsigned int m_successfulCount;
        std::vector<unsigned long> m_successfulDurations;
        std::vector<unsigned long> m_successfulRowCounts;
    };
}