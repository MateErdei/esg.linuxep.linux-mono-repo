// Copyright 2022-2023 Sophos Limited. All rights reserved.

#include "ScanRequestHandler.h"

#include "Logger.h"

#include "common/PluginUtils.h"
#include "common/SaferStrerror.h"
#include "common/StringUtils.h"
#include "datatypes/sophos_filesystem.h"
#include "datatypes/Time.h"
#include "unixsocket/threatDetectorSocket/ScanningClientSocket.h"

// Base
#include "Common/Logging/LoggerConfig.h"

#include <chrono>
#include <memory>
#include <fstream>
#include <sstream>
#include <utility>

#include <fcntl.h>

namespace fs = sophos_filesystem;
using namespace sophos_on_access_process::onaccessimpl;
using namespace scan_messages;

ScanRequestHandler::ScanRequestHandler(
   ScanRequestQueueSharedPtr scanRequestQueue,
    IScanningClientSocketSharedPtr socket,
    fanotifyhandler::IFanotifyHandlerSharedPtr fanotifyHandler,
    mount_monitor::mountinfo::IDeviceUtilSharedPtr deviceUtil,
    onaccessimpl::onaccesstelemetry::IOnAccessTelemetryUtilitySharedPtr telemetryUtility,
    int handlerId,
    sophos_on_access_process::local_settings::OnAccessLocalSettings localSettings)
    : m_scanRequestQueue(std::move(scanRequestQueue))
    , m_socket(std::move(socket))
    , m_fanotifyHandler(std::move(fanotifyHandler))
    , m_deviceUtil(std::move(deviceUtil))
    , m_telemetryUtility(std::move(telemetryUtility))
    , m_handlerId(handlerId)
    , m_localSettings(localSettings)
{
}

void ScanRequestHandler::scan(
    const scan_request_ptr_t& scanRequest,
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
        LOGWARN("ScanRequestHandler-" << m_handlerId << " scan aborted: " << e.what());
        throw;
    }
    catch (const std::exception& e)
    {
        std::string fileToScanPath(common::escapePathForLogging(scanRequest->getPath()));
        LOGERROR("ScanRequestHandler-" << m_handlerId << " failed to scan " << fileToScanPath << " : " << e.what());
        m_socketWrapper.reset();
        return;
    }

    std::string errorMsg = common::toUtf8(response.getErrorMsg());
    if (!errorMsg.empty())
    {
        LOGERROR("ScanRequestHandler-" << m_handlerId << " received error: " << errorMsg);
    }
    m_telemetryUtility->incrementFilesScanned(!errorMsg.empty());

    auto detections = response.getDetections();
    auto scanType = scan_messages::getScanTypeAsStr(scanRequest->getScanType());
    if (detections.empty() && errorMsg.empty())
    {
        if (!scanRequest->isCached())
        {
            if (m_deviceUtil->isCachable(scanRequest->getFd()))
            {
                // Clean file, ret either 0 or 1 errno is logged by m_fanotifyHandler->cacheFd
                LOGDEBUG("ScanRequestHandler-" << m_handlerId << " caching " << common::escapePathForLogging(scanRequest->getPath()) << " (" << scanType << ")");
                int ret = m_fanotifyHandler->cacheFd(scanRequest->getFd(), scanRequest->getPath(), false);
                if (ret < 0)
                {
                    std::string escapedPath(common::escapePathForLogging(scanRequest->getPath()));
                    LOGFATAL("ScanRequestHandler-" << m_handlerId << " caching " << escapedPath << " failed. Restarting On Access");
                    std::exit(EXIT_FAILURE);
                }
            }
            else
            {
                LOGDEBUG("ScanRequestHandler-" << m_handlerId <<
                         " not caching " << common::escapePathForLogging(scanRequest->getPath()) << " (" << scanType
                                   << "), as it is on a mutable mount");
            }
        }
    }
    else if (m_localSettings.uncacheDetections)
    {
        LOGDEBUG("ScanRequestHandler-" << m_handlerId << " un-caching " << common::escapePathForLogging(scanRequest->getPath())<< " (" << scanType << ")");
        // file may not have been cached, so we ignore the return code
        std::ignore = m_fanotifyHandler->uncacheFd(scanRequest->getFd(), scanRequest->getPath());

        for(const auto& detection : detections)
        {
            std::string escapedPath(common::escapePathForLogging(detection.path));
            std::string threatName = detection.name;

            LOGWARN("ScanRequestHandler-" << m_handlerId << " detected \"" << escapedPath << "\" is infected with " << threatName << " (" << scanType << ")");
        }
    }
}

void ScanRequestHandler::run()
{
    announceThreadStarted();

    LOGDEBUG("Starting ScanRequestHandler-" << m_handlerId);
    std::ofstream perfDump;
    if (m_localSettings.dumpPerfData)
    {
        std::stringstream perfDumpFileName;
        perfDumpFileName << "perfDumpThread" << m_handlerId << '_' << datatypes::Time::currentToDateTimeString("%Y-%m-%d_%H-%M-%S");
        fs::path perfDumpFilePath = common::getPluginInstallPath() / "var" / perfDumpFileName.str();
        perfDump.open(perfDumpFilePath);
        perfDump << "Scan duration\tIn-Product duration\tQueue size\tFile path" << std::endl;
    }

    auto logLevel = getOnAccessImplLogger().getChainedLogLevel();
    try
    {
        while (!stopRequested())
        {
            auto queueItem = m_scanRequestQueue->pop();
            if(queueItem)
            {
                if(logLevel <= Common::Logging::TRACE || m_localSettings.dumpPerfData)
                {
                    std::string escapedPath(common::escapePathForLogging(queueItem->getPath()));
                    LOGTRACE("ScanRequestHandler-" << m_handlerId << " picked up scan request for " << escapedPath << " with UID " << queueItem->getUserId());
                    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

                    scan(queueItem);

                    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
                    auto scanDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
                    auto inProductDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - queueItem->getCreationTime()).count();

                    if (logLevel <= Common::Logging::TRACE)
                    {
                        LOGTRACE(
                            "Scan for " << escapedPath << " completed in " << scanDuration << "ms by scanHandler-"
                                        << m_handlerId << ": Time in product is " << inProductDuration
                                        << "ms. Queue size at time of insert was "
                                        << queueItem->getQueueSizeAtTimeOfInsert());
                    }

                    if (m_localSettings.dumpPerfData)
                    {
                        perfDump << scanDuration << '\t' << inProductDuration << '\t' << queueItem->getQueueSizeAtTimeOfInsert() << '\t' << escapedPath << std::endl;
                    }
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
    LOGDEBUG("Finished ScanRequestHandler-" << m_handlerId);
}
