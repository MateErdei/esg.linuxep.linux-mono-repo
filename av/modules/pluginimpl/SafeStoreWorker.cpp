// Copyright 2022 Sophos Limited. All rights reserved.

#include "SafeStoreWorker.h"

#include "Logger.h"

#include "common/NotifyPipeSleeper.h"
#include "common/StringUtils.h"
#include "mount_monitor/mountinfoimpl/MountFactory.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "scan_messages/QuarantineResponse.h"

#include <utility>

using namespace Plugin;

SafeStoreWorker::SafeStoreWorker(IDetectionHandler &detectionHandler,
                                 std::shared_ptr<DetectionQueue> detectionQueue,
                                 fs::path safeStoreSocket,
                                 mount_monitor::mountinfo::IMountFactorySharedPtr mountFactory) :
        m_detectionHandler(detectionHandler),
        m_detectionQueue(std::move(detectionQueue)),
        m_safeStoreSocket(std::move(safeStoreSocket)),
        mountFactory_(mountFactory)
{
    LOGDEBUG("SafeStore socket path " << safeStoreSocket);
}

SafeStoreWorker::SafeStoreWorker(
    IDetectionHandler& detectionHandler,
    std::shared_ptr<DetectionQueue> detectionQueue,
    fs::path safeStoreSocket) :
        SafeStoreWorker(detectionHandler, std::move(detectionQueue), std::move(safeStoreSocket),
    std::make_shared<mount_monitor::mountinfoimpl::MountFactory>(
            std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>()
            ))
{
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
        threatDetected.quarantineResult = common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE;

        bool tryQuarantine = true;
        try
        {
            auto mountInfo = mountFactory_->newMountInfo();
            auto parentMount = mountInfo->getMountFromPath(threatDetected.filePath);
            if (parentMount != nullptr)
            {
                const std::string escapedPath = common::escapePathForLogging(threatDetected.filePath);
                if (parentMount->isNetwork())
                {
                    LOGINFO(
                        "File at location: " << escapedPath << " is located on a Network mount: "
                                             << parentMount->mountPoint() << ". Will not quarantine.");
                    threatDetected.isRemote = true;
                    tryQuarantine = false;
                }
                else if (parentMount->isReadOnly())
                {
                    LOGINFO(
                        "File at location: " << escapedPath << " is located on a ReadOnly mount: "
                                             << parentMount->mountPoint() << ". Will not quarantine.");
                    tryQuarantine = false;
                }
            }
        }
        catch (std::runtime_error& error)
        {
            LOGWARN(
                "Unable to determine detection's parent mount, due to: " << common::escapePathForLogging(error.what())
                                                                         << ". Will continue quarantine attempt.");
        }

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
                    m_detectionHandler.markAsQuarantining(threatDetected);
                    safeStoreClient.sendQuarantineRequest(threatDetected);
                }
                catch (const std::exception& e)
                {
                    // Only a warning because we will report the failure on Central
                    LOGWARN("Failed to send detection to SafeStore: " << e.what());
                }

                try
                {
                    threatDetected.quarantineResult = safeStoreClient.waitForResponse();
                }
                catch (const std::exception& e)
                {
                    // Only a warning because we will report the failure on Central
                    LOGWARN("Failed to receive a response from SafeStore: " << e.what());
                }
            }

            const std::string escapedPath = common::pathForLogging(threatDetected.filePath);
            if (threatDetected.quarantineResult == common::CentralEnums::QuarantineResult::SUCCESS)
            {
                LOGINFO("Threat cleaned up at path: " << escapedPath);
            }
            else
            {
                LOGWARN("Quarantine failed for threat: " << escapedPath);
            }
        }

        m_detectionHandler.finaliseDetection(threatDetected);
    }
    sleeper.reset();
}
