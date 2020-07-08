/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "NamedScanConfig.h"

#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"
#include "scan_messages/ThreatDetected.h"

using namespace scan_messages;
namespace avscanner::avscannerimpl
{
    using path = sophos_filesystem::path;

    class IScanCallbacks
    {
    public:
        virtual void cleanFile(const path&) = 0;
        virtual void infectedFile(const path&, const std::string& threatName) = 0;
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
        scan_messages::ScanResponse scan(const path& fileToScanPath);

    private:
        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<IScanCallbacks> m_callbacks;
        bool m_scanInArchives;
        E_SCAN_TYPE m_scanType;
    };
}

