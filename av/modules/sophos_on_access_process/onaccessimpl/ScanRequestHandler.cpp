//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include "common/PathUtils.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"

#include <sstream>

using namespace sophos_on_access_process::onaccessimpl;
namespace fs = sophos_filesystem;

ScanRequestHandler::ScanRequestHandler(
   ScanRequestQueueSharedPtr scanRequestQueue,
    std::shared_ptr<unixsocket::IScanningClientSocket> socket,
    fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler)
    : m_scanRequestQueue(scanRequestQueue)
    , m_socket(socket)
    , m_fanotifyHandler(std::move(fanotifyHandler))
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

void ScanRequestHandler::scan(
    scan_messages::ClientScanRequestPtr scanRequest,
    const struct timespec& retryInterval)
{
    std::string fileToScanPath = scanRequest->getPath();

    ScanResponse response;
    try
    {
        ClientSocketWrapper socketWrapper(*m_socket, m_notifyPipe, retryInterval);
        response = socketWrapper.scan(scanRequest);
    }
    catch (const ScanInterruptedException& e)
    {
        LOGWARN("Scan aborted: " << e.what());
    }
    catch (const std::exception& e)
    {
        LOGERROR("Failed to scan " << fileToScanPath << " : " << e.what());
        return;
    }

    std::string errorMsg = common::toUtf8(response.getErrorMsg());
    if (response.getDetections().empty())
    {
        if (!errorMsg.empty())
        {
            LOGERROR(errorMsg);
        }
        else
        {
            // Clean file
        }
    }
    else
    {
        if (!errorMsg.empty())
        {
            LOGERROR(errorMsg);
        }

        auto scanType = scan_messages::getScanTypeAsStr(scanRequest->getScanType());

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
                                  << threatName << " (" << scanType << ")");
            }
            else
            {
                LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName << " (" << scanType << ")");
            }
        }

        if (scanRequest->isOpenEvent())
        {
            int ret = m_fanotifyHandler->cacheFd(FAN_MARK_ADD | FAN_MARK_IGNORED_MASK, FAN_OPEN, scanRequest->getFd(), "");
            if (ret < 0)
            {
                LOGDEBUG("adding cache mark failed: " << ret);
            }
            else
            {
                LOGDEBUG("fanotify ignored mask set for: " << scanRequest->getFd());
            }
        }
    }
}

void ScanRequestHandler::run()
{
    announceThreadStarted();

    LOGDEBUG("Entering Main Loop");
    while (!stopRequested())
    {
        auto queueItem = m_scanRequestQueue->pop();
        if(queueItem)
        {
            scan(queueItem);
        }
    }
}
