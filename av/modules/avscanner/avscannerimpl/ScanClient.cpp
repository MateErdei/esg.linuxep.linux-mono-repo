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

static void internal_scan(unixsocket::ScanningClientSocket& socket, const sophos_filesystem::path& p)
{
    PRINT("Scanning " << p);
    int file_fd = ::open(p.c_str(), O_RDONLY);
    assert(file_fd >= 0);

    auto response = socket.scan(file_fd, p); // takes ownership of file_fd

    if (response.clean())
    {
        PRINT(p << " is fake clean");
    }
    else
    {
        PRINT(p << " is fake infected");
    }
}
using namespace avscanner::avscannerimpl;

ScanClient::ScanClient(unixsocket::ScanningClientSocket& socket)
    : m_socket(socket)
{
}

void ScanClient::scan(const sophos_filesystem::path& p)
{
    ::internal_scan(m_socket, p);
}
