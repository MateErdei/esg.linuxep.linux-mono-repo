/******************************************************************************************************

Copyright 2020, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include "ScanClient.h"

#include "Logger.h"

#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/IScanningClientSocket.h"

#include <Common/ApplicationConfiguration/IApplicationConfiguration.h>
#include <common/StringUtils.h>
#include <common/ErrorCodes.h>

#include <iostream>
#include <string>
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

std::string ScanClient::failedToOpen(const int error)
{
    switch (error)
    {
        case EACCES:
            return "Failed to open as permission denied: ";
        case ENAMETOOLONG:
            return "Failed to open as the path is too long: ";
        case ELOOP:
            return "Failed to open as couldn't follow the symlink further: ";
        case ENODEV:
            return "Failed to open as path is a device special file and no corresponding device exists: ";
        case ENOENT:
            return "Failed to open as path is a dangling symlink or a directory component is missing: ";
        case ENOMEM:
            return "Failed to open due to a lack of kernel memory: ";
        case EOVERFLOW:
            return "Failed to open as the file is too large: ";
        default:
            return "Failed to open with error " + std::to_string(error) + ": ";
    }
}

scan_messages::ScanResponse ScanClient::scan(const sophos_filesystem::path& fileToScanPath, bool isSymlink)
{
    datatypes::AutoFd file_fd(::open(fileToScanPath.c_str(), O_RDONLY));
    if (!file_fd.valid())
    {
        int error = errno;
        std::string escapedPath = common::escapePathForLogging(fileToScanPath, true);

        std::ostringstream logMsg;
        logMsg << failedToOpen(error) << escapedPath;

        m_callbacks->scanError(logMsg.str());

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
        std::string errorMsg = common::toUtf8(response.getErrorMsg());
        if (response.getDetections().empty())
        {
            if (!errorMsg.empty())
            {
                m_callbacks->scanError(errorMsg);

                if(common::contains(errorMsg, "as it is password protected"))
                {
                    m_callbacks->m_returnCode = common::E_PASSWORD_PROTECTED;
                }
            }
            else
            {
                m_callbacks->cleanFile(fileToScanPath);
            }
        }
        else
        {
            if (!errorMsg.empty())
            {
                m_callbacks->scanError(errorMsg);
            }
            std::map<path, std::string> detections;
            for(const auto& detection : response.getDetections())
            {
                detections.emplace(detection.path, detection.name);
            }

            m_callbacks->infectedFile(detections, fileToScanPath, isSymlink);
        }
    }
    return response;
}
