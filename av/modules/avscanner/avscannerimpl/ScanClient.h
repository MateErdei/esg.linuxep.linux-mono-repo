/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "NamedScanConfig.h"

#include "unixsocket/IScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"

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
        explicit ScanClient(unixsocket::IScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks, NamedScanConfig& config);
        explicit ScanClient(unixsocket::IScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks, bool scanInArchives);

        /**
         *
         * Calls IScanCallbacks if provided
         *
         * @param p Path to open and scan
         * @return Scan response
         */
        scan_messages::ScanResponse scan(const path& p);

    private:
        unixsocket::IScanningClientSocket& m_socket;
        std::shared_ptr<IScanCallbacks> m_callbacks;
        bool m_scanInArchives;
    };
}

