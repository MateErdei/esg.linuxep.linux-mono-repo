// Copyright 2020-2022, Sophos Limited.  All rights reserved.

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

        void cleanFile(const path&) override;
        void infectedFile(const std::map<path, std::string>& detections, const sophos_filesystem::path&, const std::string& scanType, bool isSymlink) override;
        void scanError(const std::string& errorMsg, std::error_code) override;
        common::E_ERROR_CODES returnCode();
        void scanStarted() override { m_startTime = time(nullptr); }
        void logSummary() override;

    protected:
        [[nodiscard]] time_t  getStartTime() const { return m_startTime; }
        [[nodiscard]] int getNoOfInfectedFiles() const { return m_noOfInfectedFiles; }
        [[nodiscard]] int getNoOfScanErrors() const { return m_noOfErrors; }
        [[nodiscard]] int getNoOfCleanFiles() const { return m_noOfCleanFiles; }
        [[nodiscard]] int getNoOfScannedFiles() const { return m_noOfCleanFiles + m_noOfInfectedFiles; }
        [[nodiscard]] std::map<std::string, int> getThreatTypes() { return m_threatCounter; }

        void incrementInfectedFileCount() { m_noOfInfectedFiles++; }
        void incrementCleanFileCount() { m_noOfCleanFiles++; }
        void incrementErrorCount() { m_noOfErrors++; }
        void addThreat(const std::string& threatName) { ++m_threatCounter[threatName]; }

    private:
        int m_noOfInfectedFiles = 0;
        int m_noOfCleanFiles = 0;
        int m_noOfErrors = 0;
        time_t  m_startTime = 0;
        std::map<std::string, int> m_threatCounter;
    };
}