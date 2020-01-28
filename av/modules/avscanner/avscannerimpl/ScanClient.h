/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "unixsocket/ScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"

namespace avscanner::avscannerimpl
{
    class ScanClient
    {
    public:
        explicit ScanClient(unixsocket::ScanningClientSocket& socket);
        void scan(const sophos_filesystem::path& p);
    private:
        unixsocket::ScanningClientSocket& m_socket;
    };
}

