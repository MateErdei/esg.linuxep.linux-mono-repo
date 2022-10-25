// Copyright 2022, Sophos Limited.  All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "scan_messages/QuarantineResponse.h"

#include "common/ThreadRunner.h"

#include <utility>

using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(
    const IDetectionReportProcessor& pluginAdapter,
    std::shared_ptr<DetectionQueue> detectionQueue,
    const fs::path& safeStoreSocket) :
    m_pluginAdapter(pluginAdapter), m_detectionQueue(std::move(detectionQueue)), m_safeStoreSocket(safeStoreSocket)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

void SafeStoreWorker::run()
{
    LOGDEBUG("Starting SafeStoreWorker");

    announceThreadStarted();

    while (true)
    {
        std::optional<scan_messages::ThreatDetected> task = m_detectionQueue->pop();

        if (stopRequested() || !task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ThreatDetected threatDetected = std::move(task).value();

        unixsocket::SafeStoreClient safeStoreClient(m_safeStoreSocket);
        safeStoreClient.sendQuarantineRequest(threatDetected);
        scan_messages::QuarantineResult quarantineResult = safeStoreClient.waitForResponse();

        if (quarantineResult == scan_messages::QUARANTINE_SUCCESS)
        {
            threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_CLEANED_UP;
            LOGDEBUG("Quarantine succeeded");
        }
        else
        {
            threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
            LOGINFO("Quarantine failed");
        }

        m_pluginAdapter.processDetectionReport(threatDetected);
    }
}
