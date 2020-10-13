/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "NamedScanConfig.h"

#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ThreatDetected.h"

#include <chrono>

using namespace scan_messages;
namespace avscanner::avscannerimpl
{
    using path = sophos_filesystem::path;
    using timestamp = std::chrono::time_point<std::chrono::system_clock>;

    class IScanCallbacks
    {
    public:
        virtual ~IScanCallbacks() = default;
        virtual void cleanFile(const path&) = 0;
        virtual void infectedFile(const path&, const std::string& threatName, bool isSymlink) = 0;
        virtual void scanError(const std::string&) = 0;
        void scanStarted() { m_startTime = std::chrono::system_clock::now(); }
        virtual void logSummary() = 0;

    protected:
        [[nodiscard]] timestamp getStartTime() { return m_startTime; };
        [[nodiscard]] int getNoOfInfectedFiles() { return m_noOfInfectedFiles; };
        [[nodiscard]] int getNoOfCleanFiles() { return m_noOfCleanFiles; };
        [[nodiscard]] int getNoOfScannedFiles() { return m_noOfCleanFiles + m_noOfInfectedFiles; };
        void incrementInfectedCount() { m_noOfInfectedFiles++; };
        void incrementCleanCount() { m_noOfCleanFiles++; };

    private:
        int m_noOfInfectedFiles = 0;
        int m_noOfCleanFiles = 0;
        timestamp m_startTime;

    };

    class ScanClient
    {
    public:
        ScanClient(const ScanClient&) = delete;
        ScanClient(ScanClient&&) = default;
        explicit ScanClient(unixsocket::IScanningClientSocket& socket,
                std::shared_ptr<IScanCallbacks> callbacks,
                bool scanInArchives,
                E_SCAN_TYPE scanType);

        /**
         *
         * Calls IScanCallbacks if provided
         *
         * @param p Path to open and scan
         * @return Scan response
         */
        scan_messages::ScanResponse scan(const path& fileToScanPath, bool isSymlink=false);

    private:
        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<IScanCallbacks> m_callbacks;
        bool m_scanInArchives;
        E_SCAN_TYPE m_scanType;
    };
}

