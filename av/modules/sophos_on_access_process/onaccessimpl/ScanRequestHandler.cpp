//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"

#include "common/PathUtils.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include <sstream>

using namespace sophos_on_access_process::onaccessimpl;
namespace fs = sophos_filesystem;

ScanRequestHandler::ScanRequestHandler(
   ScanRequestQueueSharedPtr scanRequestQueue,
    std::shared_ptr<unixsocket::IScanningClientSocket> socket,
    fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler)
    : m_scanRequestQueue(std::move(scanRequestQueue))
    , m_socket(std::move(socket))
    , m_fanotifyHandler(std::move(fanotifyHandler))
{
}

void ScanRequestHandler::scan(
    const scan_messages::ClientScanRequestPtr& scanRequest,
    const struct timespec& retryInterval)
{
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
        std::string fileToScanPath(common::escapePathForLogging(scanRequest->getPath()));
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
            if (scanRequest->isOpenEvent())
            {
                int ret = m_fanotifyHandler->cacheFd(scanRequest->getFd(), scanRequest->getPath());
                if (ret < 0)
                {
                    int error = errno;
                    std::string escapedPath(common::escapePathForLogging(scanRequest->getPath()));
                    LOGWARN("Caching " << escapedPath << " failed: " << common::safer_strerror(error));
                }
            }
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

            LOGWARN("Detected \"" << escapedPath << "\" is infected with " << threatName << " (" << scanType << ")");
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
