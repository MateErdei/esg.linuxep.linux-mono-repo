/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "unixsocket/ScanningClientSocket.h"
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
        explicit ScanClient(unixsocket::ScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks);
        void scan(const path& p);
    private:
        unixsocket::ScanningClientSocket& m_socket;
        std::shared_ptr<IScanCallbacks> m_callbacks;
    };
}

