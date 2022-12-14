// Copyright 2022 Sophos Limited. All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "common/NotifyPipeSleeper.h"
#include "common/StringUtils.h"
#include "mount_monitor/mountinfoimpl/Drive.h"
#include "scan_messages/QuarantineResponse.h"

#include <utility>

using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(
    const IDetectionReportProcessor& pluginAdapter,
    IDetectionDatabaseHandler& databaseHandler,
    std::shared_ptr<DetectionQueue> detectionQueue,
    const fs::path& safeStoreSocket) :
    m_pluginAdapter(pluginAdapter),
    m_databaseHandler(databaseHandler),
    m_detectionQueue(std::move(detectionQueue)),
    m_safeStoreSocket(safeStoreSocket)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

void SafeStoreWorker::run()
{
    LOGDEBUG("Starting SafeStoreWorker");

    announceThreadStarted();

    common::IStoppableSleeperSharedPtr sleeper = std::make_shared<common::NotifyPipeSleeper>(m_notifyPipe);

    while (true)
    {
        std::optional<scan_messages::ThreatDetected> task = m_detectionQueue->pop();

        if (stopRequested() || !task.has_value())
        {
            LOGDEBUG("Got stop request, exiting SafeStoreWorker");
            break;
        }

        scan_messages::ThreatDetected threatDetected = std::move(task).value();

        bool tryQuarantine = true;
        try
        {
            auto parentMount = mount_monitor::mountinfoimpl::Drive(threatDetected.filePath);
            if (parentMount.isNetwork())
            {
                LOGINFO("File is located on a Network mount: " << parentMount.mountPoint() << ". Will not quarantine.");
                threatDetected.isRemote = true;
                threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
                tryQuarantine = false;
            }
            else if (parentMount.isReadOnly())
            {
                LOGINFO(
                    "File is located on a ReadOnly mount: " << parentMount.mountPoint() << ". Will not quarantine.");
                threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
                tryQuarantine = false;
            }
        }
        catch (std::runtime_error& error)
        {
            LOGWARN(
                "Unable to determine detection's parent mount, due to: " << common::escapePathForLogging(error.what())
                                                                         << ". Will continue quarantine attempt.");
        }

        auto quarantineResult = common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;
        if (tryQuarantine)
        {
            unixsocket::SafeStoreClient safeStoreClient(
                m_safeStoreSocket, m_notifyPipe, unixsocket::SafeStoreClient::DEFAULT_SLEEP_TIME, sleeper);

            if (!safeStoreClient.isConnected())
            {
                LOGWARN("Failed to connect to SafeStore");
            }
            else
            {
                try
                {
                    safeStoreClient.sendQuarantineRequest(threatDetected);
                }
                catch (const std::exception& e)
                {
                    // Only a warning because we will report the failure on Central
                    LOGWARN("Failed to send detection to SafeStore: " << e.what());
                    threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
                }

                try
                {
                    quarantineResult = safeStoreClient.waitForResponse();
                }
                catch (const std::exception& e)
                {
                    // Only a warning because we will report the failure on Central
                    LOGWARN("Failed to receive a response from SafeStore: " << e.what());
                    threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
                }
            }
            const std::string escapedPath = common::escapePathForLogging(threatDetected.filePath);
            if (quarantineResult == common::CentralEnums::QuarantineResult::SUCCESS)
            {
                threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_CLEANED_UP;

                LOGINFO("Threat cleaned up at path: " << escapedPath);
            }
            else
            {
                threatDetected.notificationStatus = scan_messages::E_NOTIFICATION_STATUS_NOT_CLEANUPABLE;
                LOGWARN("Quarantine failed for threat: " << escapedPath);
            }
        }

        m_pluginAdapter.processDetectionReport(threatDetected, quarantineResult);
        m_databaseHandler.updateThreatDatabase(threatDetected);
    }
    sleeper.reset();
}
