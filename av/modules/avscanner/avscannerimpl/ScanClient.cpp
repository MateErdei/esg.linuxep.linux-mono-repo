/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <common/StringUtils.h>

#include <iostream>
#include <fcntl.h>

using namespace avscanner::avscannerimpl;
namespace fs = sophos_filesystem;

ScanClient::ScanClient(unixsocket::IScanningClientSocket& socket,
                       std::shared_ptr<IScanCallbacks> callbacks,
                       bool scanInArchives,
                       E_SCAN_TYPE scanType)
        : m_socket(socket)
        , m_callbacks(std::move(callbacks))
        , m_scanInArchives(scanInArchives)
        , m_scanType(scanType)
{
}

scan_messages::ScanResponse ScanClient::scan(const sophos_filesystem::path& fileToScanPath, bool isSymlink)
{
    datatypes::AutoFd file_fd(::open(fileToScanPath.c_str(), O_RDONLY));
    if (!file_fd.valid())
    {
        LOGERROR("Failed to open: "<< common::toUtf8(fileToScanPath));
        return scan_messages::ScanResponse();
    }

    scan_messages::ClientScanRequest request;
    request.setPath(fileToScanPath);
    request.setScanInsideArchives(m_scanInArchives);
    request.setScanType(m_scanType);
    const char* user = std::getenv("USER");
    request.setUserID(user ? user : "root");

    auto response = m_socket.scan(file_fd, request);

    if (m_callbacks)
    {
        std::string errorMsg = response.getErrorMsg();
        if (!errorMsg.empty())
        {
            m_callbacks->scanError(errorMsg);
        }
        else
        {
            if (response.getDetections().empty())
            {
                m_callbacks->cleanFile(fileToScanPath);
            }
            else
            {
                for (const auto &detection : response.getDetections())
                {
                    m_callbacks->infectedFile(detection.first, detection.second, isSymlink);
                }
            }
        }
    }
    return response;
}
