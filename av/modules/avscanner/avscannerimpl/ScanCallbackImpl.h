/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseFileWalkCallbacks.h"
#include "ScanClient.h"
#include <chrono>
#include <map>
#include <time.h>

namespace avscanner::avscannerimpl
{
    using timestamp = std::chrono::time_point<std::chrono::system_clock>;

    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        void cleanFile(const path&) override;
        void infectedFile(const path& path, const std::string& threatName, bool isSymlink=false) override;
        void scanError(const std::string& errorMsg) override;
        void scanStarted() override { m_startTime = time(nullptr); }
        void logSummary() override;

        [[nodiscard]] int returnCode() const { return m_returnCode; }

    protected:
        [[nodiscard]] time_t  getStartTime() { return m_startTime; };
        [[nodiscard]] int getNoOfThreats() { return m_threatCounter.size(); };
        [[nodiscard]] int getNoOfInfectedFiles() { return m_noOfInfectedFiles; };
        [[nodiscard]] int getNoOfCleanFiles() { return m_noOfCleanFiles; };
        [[nodiscard]] int getNoOfScannedFiles() { return m_noOfCleanFiles + m_noOfInfectedFiles; };
        [[nodiscard]] std::map<std::string, int> getThreatTypes() { return m_threatCounter; };

        void incrementInfectedCount() { m_noOfInfectedFiles++; };
        void incrementCleanCount() { m_noOfCleanFiles++; };
        void addThreat(const std::string& threatName) { ++m_threatCounter[threatName]; };

    private:
        int m_noOfInfectedFiles = 0;
        int m_noOfCleanFiles = 0;
        time_t  m_startTime;
        int m_returnCode = E_CLEAN;
        std::map<std::string, int> m_threatCounter;
    };
}