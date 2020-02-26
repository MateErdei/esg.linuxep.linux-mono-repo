/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Print.h"

#include <iostream>
#include <fcntl.h>

using namespace avscanner::avscannerimpl;

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       NamedScanConfig& config)
       : ScanClient(socket, std::move(callbacks), config.m_scanArchives)
{
}

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       bool scanInArchives)
        : m_socket(socket), m_callbacks(std::move(callbacks)), m_scanInArchives(scanInArchives)
{
}

scan_messages::ScanResponse ScanClient::scan(const sophos_filesystem::path& p)
{
    datatypes::AutoFd file_fd(::open(p.c_str(), O_RDONLY));
    if (!file_fd.valid())
    {
        PRINT("Unable to open "<<p);
        return scan_messages::ScanResponse();
    }

    scan_messages::ClientScanRequest request;
    request.setPath(p);
    request.setScanInsideArchives(m_scanInArchives);

    auto response = m_socket.scan(file_fd, request);

    if (m_callbacks)
    {
        if (response.clean())
        {
            m_callbacks->cleanFile(p);
        }
        else
        {
            m_callbacks->infectedFile(p, response.threatName());
        }
    }
    return response;
}
