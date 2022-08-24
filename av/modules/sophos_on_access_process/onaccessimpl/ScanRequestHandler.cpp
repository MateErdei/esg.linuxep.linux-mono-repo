//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "common/PathUtils.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"

#include <sstream>
#include <fcntl.h>

using namespace sophos_on_access_process::onaccessimpl;
namespace fs = sophos_filesystem;

ScanRequestHandler::ScanRequestHandler(
    std::shared_ptr<sophos_on_access_process::onaccessimpl::ScanRequestQueue> scanRequestQueue,
    const std::string& socketPath)
    : m_scanRequestQueue(scanRequestQueue)
    , m_socketPath(socketPath)
{

}

std::string ScanRequestHandler::failedToOpen(const int error)
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

void ScanRequestHandler::scan(std::shared_ptr<scan_messages::ClientScanRequest> scanRequest, int fd)
{
    std::string fileToScanPath = scanRequest->getPath();
    datatypes::AutoFd file_fd(fd);

    unixsocket::ScanningClientSocket scanningSocket(m_socketPath);
    ClientSocketWrapper socketWrapper(scanningSocket, m_notifyPipe);
    auto response = socketWrapper.scan(file_fd, *scanRequest);

    std::string errorMsg = common::toUtf8(response.getErrorMsg());
    if (response.getDetections().empty())
    {
        if (!errorMsg.empty())
        {
            LOGERROR(errorMsg);
        }
        else
        {
            LOGINFO(">>> Clean: " << fileToScanPath);
            // Clean file
        }
    }
    else
    {
        if (!errorMsg.empty())
        {
            LOGERROR(errorMsg);
        }

        for(const auto& detection : response.getDetections())
        {
            std::string escapedPath(common::escapePathForLogging(detection.path));
            std::string threatName = detection.name;

            fs::file_status symlinkStatus = fs::symlink_status(common::PathUtils::removeForwardSlashFromPath(detection.path));
            if (fs::is_symlink(symlinkStatus))
            {
                std::string escapedTargetPath(common::escapePathForLogging(fs::canonical(detection.path)));
                LOGWARN(
                    "Detected \"" << escapedPath << "\" (symlinked to " << escapedTargetPath << ") is infected with "
                                  << threatName);
            }
            else
            {
                LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName);
            }
        }
    }
}

void ScanRequestHandler::run()
{
    announceThreadStarted();

    unixsocket::ScanningClientSocket scanningSocket(m_socketPath);
    while (!stopRequested())
    {
        LOGINFO("m_scanRequestQueue->pop()");
        auto queueItem = m_scanRequestQueue->pop();
        scan(queueItem.first, queueItem.second);
    }
}
