// Copyright 2020-2023 Sophos Limited. All rights reserved.

#define PLUGIN_INTERNAL public

#define TEST_PUBLIC public

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
#include "tests/common/MockMountPoint.h"
#include "tests/safestore/MockIQuarantineManager.h"
#include "tests/scan_messages/SampleThreatDetected.h"
#include "av/modules/mount_monitor/mountinfoimpl/MountFactory.h"

#include <gtest/gtest.h>

using namespace testing;
using namespace Plugin;
using namespace scan_messages;
using namespace common::CentralEnums;

namespace
{
    using namespace mount_monitor::mountinfo;

    class FakeMountsFactory : public IMountFactory
    {
    public:
        explicit FakeMountsFactory(IMountInfoSharedPtr mountInfo)
                : mountInfo_(std::move(mountInfo))
        {}
        IMountInfoSharedPtr newMountInfo() override
        {
            assert(mountInfo_);
            return mountInfo_;
        }
        IMountInfoSharedPtr mountInfo_;
    };

    IMountFactorySharedPtr localWritableFactory()
    {
        auto mockMounts = std::make_shared<StrictMock<MockMounts>>();
        auto mockMountPoint = std::make_shared<StrictMock<MockMountPoint>>();
        EXPECT_CALL(*mockMounts, getMountFromPath(_)).WillRepeatedly(Return(mockMountPoint));
        EXPECT_CALL(*mockMountPoint, isNetwork()).WillRepeatedly(Return(false));
        EXPECT_CALL(*mockMountPoint, isReadOnly()).WillRepeatedly(Return(false));
        return std::make_shared<FakeMountsFactory>(mockMounts);
    }

    class TestSafeStoreWorker : public PluginMemoryAppenderUsingTests
    {
    protected:
        TestSafeStoreWorker() :
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

    };
} // namespace

TEST_F(TestSafeStoreWorker, ExitsWhenEmptyQueueRequestedToStop)
{
    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };

    worker.start();
    m_detectionQueue->requestStop();
    worker.join();
}

TEST_F(TestSafeStoreWorker, ExitsWhenThreadRequestedToStop)
{
    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath };

    worker.start();

    worker.requestStop();
    auto detection = createThreatDetected({});
    ASSERT_TRUE(m_detectionQueue->push(detection)); // It will not stop until it receives something on the queue

    worker.join();
}

// The following tests are more complicated than expected due to being unable to mock the client response

TEST_F(TestSafeStoreWorker, AttemptsQuarantineButCantConnectFinalisesDetection)
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

TEST_F(TestSafeStoreWorker, AttemptsQuarantineButFailsToReceiveResponse)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });

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

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath,
                            localWritableFactory()};
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}

TEST_F(TestSafeStoreWorker, AttemptsQuarantineAndReceivesSuccessFromSafeStore)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });

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

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath,
                            localWritableFactory()};
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}

TEST_F(TestSafeStoreWorker, AttemptsQuarantineAndReceivesFailureFromSafeStore)
{
    const std::string filepath = "/file";
    // Need to pass a real fd
    auto threatDetected = createThreatDetectedWithRealFd({ .filePath=filepath });

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

    SafeStoreWorker worker{ m_mockDetectionHandler, m_detectionQueue, m_socketPath,
                            localWritableFactory()};
    worker.start();

    unixsocket::SafeStoreServerSocket server{ m_socketPath, m_mockQuarantineManager };
    server.start();

    ASSERT_TRUE(m_detectionQueue->push(threatDetected));

    m_detectionQueue->requestStop();
    worker.join();
    server.requestStop();
    server.join();
}