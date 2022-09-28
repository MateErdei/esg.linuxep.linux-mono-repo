//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include <memory>
#include <sstream>

using namespace sophos_on_access_process::onaccessimpl;

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
        if (!m_socketWrapper)
        {
            m_socketWrapper = std::make_unique<ClientSocketWrapper>(*m_socket, m_notifyPipe, retryInterval);
        }
        response = m_socketWrapper->scan(scanRequest);
    }
    catch (const ScanInterruptedException& e)
    {
        LOGWARN("Scan aborted: " << e.what());
        throw;
    }
    catch (const std::exception& e)
    {
        std::string fileToScanPath(common::escapePathForLogging(scanRequest->getPath()));
        LOGERROR("Failed to scan " << fileToScanPath << " : " << e.what());
        m_socketWrapper.reset();
        return;
    }

    std::string errorMsg = common::toUtf8(response.getErrorMsg());
    if (!errorMsg.empty())
    {
        LOGERROR(errorMsg);
    }

    auto detections = response.getDetections();
    if (detections.empty())
    {
        if (errorMsg.empty() && scanRequest->isOpenEvent())
        {
            // Clean file
            int ret = m_fanotifyHandler->cacheFd(scanRequest->getFd(), scanRequest->getPath());
            if (ret < 0)
            {
                int error = errno;
                std::ignore = error; // Fuzz builds compile out LOGDEBUG
                std::string escapedPath(common::escapePathForLogging(scanRequest->getPath()));
                LOGWARN("Caching " << escapedPath << " failed: " << common::safer_strerror(error));
            }

        }
    }
    else
    {
        auto scanType = scan_messages::getScanTypeAsStr(scanRequest->getScanType());

        for(const auto& detection : detections)
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

    LOGDEBUG("Starting ScanRequestHandler");
    try
    {
        while (!stopRequested())
        {
            auto queueItem = m_scanRequestQueue->pop();
            if (queueItem)
            {
                scan(queueItem);
            }
        }
    }
    catch (const ScanInterruptedException& e)
    {
        // Aborting the scan thread - stop requested while scan waiting for response
        // Already logged
    }
    m_socketWrapper.reset();
    LOGDEBUG("Finished ScanRequestHandler");
}
