/******************************************************************************************************

Copyright 2018 Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <ctime>
#include <string>
#include <vector>

namespace Example
{
    class ScanReport {
        time_t m_startTime;
        time_t m_finishTime;
        int m_noFilesScanned;
        double m_totalMemory;
        std::vector<std::string> m_infections;

    public:
        explicit ScanReport(time_t startTime=0);
        void scanFinished(time_t endTime=0);
        void reportInfection(const std::string & filePath);
        void reportNewFileScanned(double fileSize);

        time_t getStartTime() const;
        time_t getFinishTime() const;
        int getNoFilesScanned() const;
        double getTotalMemoryMB() const;
        const std::vector<std::string> & getInfections() const;
        time_t now()const;
    };
}


