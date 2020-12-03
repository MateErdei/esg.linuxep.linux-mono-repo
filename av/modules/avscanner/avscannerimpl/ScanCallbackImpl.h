/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "BaseFileWalkCallbacks.h"
#include "ScanClient.h"

#include <chrono>
#include <map>
#include <set>

#include <time.h>

namespace avscanner::avscannerimpl
{
    using timestamp = std::chrono::time_point<std::chrono::system_clock>;

    class ScanCallbackImpl : public IScanCallbacks
    {
    public:
        ScanCallbackImpl() = default;

        void cleanFile(const path& scannedPath) override;
        void infectedFile(const path& susiPath, const std::string& threatName, const sophos_filesystem::path&, bool isSymlink=false) override;
        void scanError(const std::string& errorMsg) override;
        void scanStarted() override { m_startTime = time(nullptr); }
        void logSummary() override;

        [[nodiscard]] int returnCode() const { return m_returnCode; }

    protected:
        [[nodiscard]] time_t  getStartTime() { return m_startTime; };
        [[nodiscard]] int getNoOfThreats() { return m_threatCounter.size(); };
        [[nodiscard]] int getNoOfInfectedFiles() { return m_infectedFiles.size(); };
        [[nodiscard]] int getNoOfCleanFiles() { return m_cleanFiles.size(); };
        [[nodiscard]] int getNoOfScanErrors() { return m_noOfErrors; };
        [[nodiscard]] int getNoOfScannedFiles() { return m_cleanFiles.size() + m_infectedFiles.size(); };
        [[nodiscard]] std::map<std::string, int> getThreatTypes() { return m_threatCounter; };

//        void incrementInfectedCount() { m_noOfInfectedFiles++; };
//        void incrementCleanCount() { m_noOfCleanFiles++; };
        void incrementErrorCount() { m_noOfErrors++; };
        void addThreat(const std::string& threatName) { ++m_threatCounter[threatName]; };
        void recordCleanFile(const std::string& threatName) { m_cleanFiles.insert(threatName); };
        void recordInfectedFile(const std::string& threatName) { m_infectedFiles.insert(threatName); };

    private:
//        int m_noOfInfectedFiles = 0;
//        int m_noOfCleanFiles = 0;
        int m_noOfErrors = 0;
        time_t  m_startTime = 0;
        int m_returnCode = E_CLEAN;
        std::map<std::string, int> m_threatCounter;
        std::set<std::string> m_cleanFiles;
        std::set<std::string> m_infectedFiles;
    };
}