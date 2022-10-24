//Copyright 2022, Sophos Limited.  All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"

#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

#include <Common/Logging/LoggerConfig.h>

#include <chrono>
#include <memory>
#include <sstream>
#include <utility>
#include <fcntl.h>


using namespace sophos_on_access_process::onaccessimpl;

ScanRequestHandler::ScanRequestHandler(
   ScanRequestQueueSharedPtr scanRequestQueue,
    IScanningClientSocketSharedPtr socket,
    fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
    int handlerId,
    bool dumpPerfData)
    : m_scanRequestQueue(std::move(scanRequestQueue))
    , m_socket(std::move(socket))
    , m_fanotifyHandler(std::move(fanotifyHandler))
    , m_handlerId(handlerId)
    , m_dumpPerfData(dumpPerfData)
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
    auto scanType = scan_messages::getScanTypeAsStr(scanRequest->getScanType());
    if (detections.empty() && errorMsg.empty())
    {
        // Clean file, ret either 0 or 1 errno is logged by m_fanotifyHandler->cacheFd
        LOGDEBUG("Caching " << common::escapePathForLogging(scanRequest->getPath())<< " (" << scanType << ")");
        int ret = m_fanotifyHandler->cacheFd(scanRequest->getFd(), scanRequest->getPath());
        if (ret < 0)
        {
            std::string escapedPath(common::escapePathForLogging(scanRequest->getPath()));
            LOGFATAL("Caching " << escapedPath << " failed. Restarting On Access");
            std::exit(EXIT_FAILURE);
        }
    }
    else
    {
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
    auto logLevel = getOnAccessImplLogger().getChainedLogLevel();
    try
    {
        while (!stopRequested())
        {
            auto queueItem = m_scanRequestQueue->pop();
            if(queueItem)
            {
                if(logLevel == Common::Logging::TRACE || m_dumpPerfData)
                {
                    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

                    scan(queueItem);

                    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                    auto scanDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                    auto inProductDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - queueItem->getCreationTime()).count();

                    std::string escapedPath(common::escapePathForLogging(queueItem->getPath()));
                    LOGTRACE("Scan for " << escapedPath << " completed in " << scanDuration << "ms by scanHandler-" << m_handlerId
                                         << ": Time in product is " << inProductDuration
                                         << "ms. Queue size at time of insert was " << queueItem->getQueueSizeAtTimeOfInsert());
                }
                else
                {
                    scan(queueItem);
                }
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
