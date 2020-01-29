/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "unixsocket/ScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <iostream>
#include <cassert>
#include <fcntl.h>

using namespace avscanner::avscannerimpl;

ScanClient::ScanClient(unixsocket::ScanningClientSocket& socket, std::shared_ptr<IScanCallbacks> callbacks)
    : m_socket(socket), m_callbacks(std::move(callbacks))
{
}

void ScanClient::scan(const sophos_filesystem::path& p)
{
    int file_fd = ::open(p.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    auto response = m_socket.scan(file_fd, p); // takes ownership of file_fd

    if (response.clean())
    {
        m_callbacks->cleanFile(p);
    }
    else
    {
        m_callbacks->infectedFile(p, response.threatName());
    }
}
