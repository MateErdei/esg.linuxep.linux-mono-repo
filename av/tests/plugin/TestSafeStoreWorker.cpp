// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define PLUGIN_INTERNAL public

// product includes
#include "common/ApplicationPaths.h"
#include "mount_monitor/mountinfoimpl/Mounts.h"
#include "mount_monitor/mountinfoimpl/SystemPathsFactory.h"
#include "pluginimpl/DetectionQueue.h"
#include "pluginimpl/SafeStoreWorker.h"
#include "unixsocket/safeStoreSocket/SafeStoreServerSocket.h"

#include "Common/ApplicationConfiguration/IApplicationConfiguration.h"

// Test includes
#include "MockDetectionHandler.h"
#include "PluginMemoryAppenderUsingTests.h"

#include "tests/common/SetupFakePluginDir.h"
#include "tests/safestore/MockIQuarantineManager.h"
#include "tests/scan_messages/SampleThreatDetected.h"

#include <gtest/gtest.h>

using namespace testing;
using namespace Plugin;
using namespace scan_messages;
using namespace common::CentralEnums;

namespace
{
    class TestSafeStoreSocket : public PluginMemoryAppenderUsingTests
    {
    protected:
        TestSafeStoreSocket() :
            m_tempDir{ setupFakePluginDir() },
            m_detectionQueue{ std::make_shared<DetectionQueue>() },
            m_memoryAppenderHolder{ *this },
            m_socketPath{ Plugin::getSafeStoreSocketPath() },
            m_mockQuarantineManager{ std::make_shared<MockIQuarantineManager>() }
        {
        }

        std::unique_ptr<Tests::TempDir> m_tempDir;
        std::shared_ptr<DetectionQueue> m_detectionQueue;
        UsingMemoryAppender m_memoryAppenderHolder;
        MockDetectionHandler m_mockDetectionHandler;
        const std::string m_socketPath;
        std::shared_ptr<MockIQuarantineManager> m_mockQuarantineManager;

        bool mountIsWritable(const std::string& filePath)
        {
            static auto pathsFactory = std::make_shared<mount_monitor::mountinfoimpl::SystemPathsFactory>();
            static auto mountInfo = std::make_shared<mount_monitor::mountinfoimpl::Mounts>(
                    pathsFactory->createSystemPaths());
            auto parentMount = mountInfo->getMountFromPath(filePath);
            return !parentMount->isReadOnly();
        }

        void assertMountIsWritable(const std::string& filePath)
        {
            ASSERT_TRUE(mountIsWritable(filePath));
        }
    };
} // namespace

TEST_F(TestSafeStoreSocket, ExitsWhenEmptyQueueRequestedToStop)
{
    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };

    worker.start();
    m_detectionQueue->requestStop();
    worker.join();
}

TEST_F(TestSafeStoreSocket, ExitsWhenThreadRequestedToStop)
{
    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };

    worker.start();

    worker.requestStop();
    auto detection = createThreatDetected({});
    ASSERT_TRUE(m_detectionQueue->push(detection)); // It will not stop until it receives something on the queue

    worker.join();
}

// The following tests are more complicated than expected due to being unable to mock the client response

TEST_F(TestSafeStoreSocket, AttemptsQuarantineButCantConnectFinalisesDetection)
{
    EXPECT_CALL(m_mockDetectionHandler, markAsQuarantining(_)).Times(0);
    EXPECT_CALL(
        m_mockDetectionHandler,
        finaliseDetection(
            Field(&ThreatDetected::quarantineResult, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE)))
        .Times(1);

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };
    worker.start();

    auto detection = createThreatDetected({});
    ASSERT_TRUE(m_detectionQueue->push(detection));

    m_detectionQueue->requestStop();
    worker.join();
}

TEST_F(TestSafeStoreSocket, AttemptsQuarantineButFailsToReceiveResponse)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });
    if (!mountIsWritable(threatDetected.filePath))
    {
        GTEST_SKIP() << "Can't test if / is read-only";
    }

    {
        // We expect it to mark the detection, and then finalise it with failure
        InSequence seq;
        EXPECT_CALL(m_mockDetectionHandler, markAsQuarantining(_)).Times(1);
        EXPECT_CALL(
            m_mockDetectionHandler,
            finaliseDetection(Field(
                &ThreatDetected::quarantineResult, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE)))
            .Times(1);
    }

    // Needed so SafeStore server connection doesn't send back response
    ON_CALL(*m_mockQuarantineManager, quarantineFile)
        .WillByDefault(Throw(std::runtime_error{ "fail so no response is sent" }));

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}

TEST_F(TestSafeStoreSocket, AttemptsQuarantineAndReceivesSuccessFromSafeStore)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });
    if (!mountIsWritable(threatDetected.filePath))
    {
        GTEST_SKIP() << "Can't test if / is read-only";
    }

    {
        // We expect it to mark the detection, and then finalise it with success
        InSequence seq;
        EXPECT_CALL(m_mockDetectionHandler, markAsQuarantining(_)).Times(1);
        EXPECT_CALL(
            m_mockDetectionHandler,
            finaliseDetection(
                Field(&ThreatDetected::quarantineResult, common::CentralEnums::QuarantineResult::SUCCESS)))
            .Times(1);
    }

    // Needed so SafeStore client socket returns success
    ON_CALL(*m_mockQuarantineManager, quarantineFile)
        .WillByDefault(Return(common::CentralEnums::QuarantineResult::SUCCESS));

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}

TEST_F(TestSafeStoreSocket, AttemptsQuarantineAndReceivesFailureFromSafeStore)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });
    if (!mountIsWritable(threatDetected.filePath))
    {
        GTEST_SKIP() << "Can't test if / is read-only";
    }

    {
        // We expect it to mark the detection, and then finalise it with failure
        InSequence seq;
        EXPECT_CALL(m_mockDetectionHandler, markAsQuarantining(_)).Times(1);
        EXPECT_CALL(
            m_mockDetectionHandler,
            finaliseDetection(Field(
                &ThreatDetected::quarantineResult, common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE)))
            .Times(1);
    }

    // Needed so SafeStore client socket returns failure
    ON_CALL(*m_mockQuarantineManager, quarantineFile)
        .WillByDefault(Return(common::CentralEnums::QuarantineResult::FAILED_TO_DELETE_FILE));

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}