// Copyright 2020-2023 Sophos Limited. All rights reserved.

#pragma once

#include "ClientSocketWrapper.h"
#include "IScanClient.h"
#include "NamedScanConfig.h"
#include "PuaExclusions.h"

#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ThreatDetected.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

using namespace scan_messages;
namespace avscanner::avscannerimpl
{
    using path = sophos_filesystem::path;

    class IScanCallbacks
    {
    public:
        virtual ~IScanCallbacks() = default;
        virtual void cleanFile(const path&) = 0;
        /**
         * @param susiPath - the path that SUSI returns after a detection
         * @param threatName
         * @param realPath - the path that's on the filesystem, it differs from the first argument path which is the path that SUSI returns
         * @param isSymlink
         */
        virtual void infectedFile(const std::map<path, std::string>& detections, const sophos_filesystem::path& realPath, const std::string& scanType, bool isSymlink) = 0;

        /**
         * Report an error related to scanning.
         *
         * @param errorCode the error_code related to the scan error.
         */
        virtual void scanError(const std::string&, std::error_code errorCode) = 0;

        virtual void scanStarted() = 0;
        virtual void logSummary() = 0;

        int m_returnCode = 0;
    };

    class ScanClient : public IScanClient
    {
    public:
        ScanClient(const ScanClient&) = delete;
        ScanClient(ScanClient&&) = default;
        explicit ScanClient(unixsocket::IScanningClientSocket& socket,
                std::shared_ptr<IScanCallbacks> callbacks,
                bool scanInArchives,
                bool scanInImages,
                bool detectPUAs,
                E_SCAN_TYPE scanType);

        void setPuaExclusions(pua_exclusion_t exclusions)
        {
            puaExclusions_ = std::move(exclusions);
        }

        /**
         *
         * Calls IScanCallbacks if provided
         *
         * @param p Path to open and scan
         * @return Scan response
         */
        scan_messages::ScanResponse scan(const sophos_filesystem::path& fileToScanPath)
        {
            return scan(fileToScanPath, false);
        }

        /**
         *
         * Calls IScanCallbacks if provided
         *
         * @param p Path to open and scan
         * @param isSymlink Has path been found via a symlink
         * @return Scan response
         */
        scan_messages::ScanResponse scan(const path& fileToScanPath, bool isSymlink) override;

        void scanError(const std::ostringstream& error, std::error_code errorCode) override
        {
            m_callbacks->scanError(error.str(), errorCode);
        }

        static std::string failedToOpen(int error);

    private:
        pua_exclusion_t puaExclusions_;
        ClientSocketWrapper m_socket;
        std::shared_ptr<IScanCallbacks> m_callbacks;
        bool m_scanInArchives;
        bool m_scanInImages;
        bool m_detectPUAs;
        E_SCAN_TYPE m_scanType;
    };
}

