// Copyright 2022-2023 Sophos Limited. All rights reserved.

#pragma once

#define TEST_PUBLIC public

#include "unixsocket/processControllerSocket/ProcessControllerServerSocket.h"

#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatDetectorControlCallback.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatDetectorResources.h"

// test headers
#include "MockMetadataRescanServerSocket.h"
#include "MockScanningServerSocket.h"
#include "MockShutdownTimer.h"
#include "MockThreatDetectorControlCallbacks.h"
#include "MockThreatReporter.h"
#include "MockUpdateCompleteServerSocket.h"

#include "tests/sophos_threat_detector/threat_scanner/MockSusiScannerFactory.h"

#include "tests/common/MockPidLock.h"
#include "tests/common/MockSignalHandler.h"

#include "Common/Helpers/MockSysCalls.h"

#include <gmock/gmock.h>

using namespace testing;
using namespace sspl::sophosthreatdetectorimpl;

namespace
{
    class MockThreatDetectorResources : public sspl::sophosthreatdetectorimpl::IThreatDetectorResources
    {
    public:
        MockThreatDetectorResources(const fs::path& testDirectory, std::shared_ptr<MockSystemCallWrapper> mockSysCallWrapper)
        {
            m_mockSysCalls = std::move(mockSysCallWrapper);
            m_mockSigTermHandler = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockUsr1Monitor = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockPidLock = std::make_shared<NiceMock<MockPidLock>>();
            m_mockThreatReporter = std::make_shared<NiceMock<MockThreatReporter>>();
            m_mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();
            m_mockUpdateCompleteServerSocket = std::make_shared<NiceMock<MockUpdateCompleteServerSocket>>(fs::path(testDirectory / "update_socket"), 0777);
            m_mockSusiScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();
            m_mockScanningServerSocket = std::make_shared<NiceMock<MockScanningServerSocket>>(fs::path(testDirectory / "scanning_server_socket"), 0777, m_mockSusiScannerFactory);
            m_reloader = std::make_shared<NiceMock<MockReloader>>();
            m_safeStoreRescanWorker = std::make_shared<NiceMock<MockSafeStoreRescanWorker>>();
            m_mockMetadataRescanServerSocket = std::make_shared<NiceMock<MockMetadataRescanServerSocket>>(fs::path(testDirectory / "metadata_rescan_socket"), 0777, m_mockSusiScannerFactory);

            auto mockThreatDetectorControlCallbacks = std::make_shared<NiceMock<MockThreatDetectorControlCallbacks>>();
            m_processControlServerSocket = std::make_shared<unixsocket::ProcessControllerServerSocket>(fs::path(testDirectory / "process_control_socket"), 0777, mockThreatDetectorControlCallbacks);

            ON_CALL(*this, createSystemCallWrapper).WillByDefault(Return(m_mockSysCalls));
            ON_CALL(*this, createSigTermHandler).WillByDefault(Return(m_mockSigTermHandler));
            ON_CALL(*this, createUsr1Monitor).WillByDefault(Return(m_mockUsr1Monitor));
            ON_CALL(*this, createPidLockFile).WillByDefault(Return(m_mockPidLock));
            ON_CALL(*this, createThreatReporter).WillByDefault(Return(m_mockThreatReporter));
            ON_CALL(*this, createShutdownTimer).WillByDefault(Return(m_mockShutdownTimer));
            ON_CALL(*this, createUpdateCompleteNotifier).WillByDefault(Return(m_mockUpdateCompleteServerSocket));
            ON_CALL(*this, createSusiScannerFactory).WillByDefault(Return(m_mockSusiScannerFactory));
            ON_CALL(*this, createScanningServerSocket).WillByDefault(Return(m_mockScanningServerSocket));
            ON_CALL(*this, createReloader).WillByDefault(Return(m_reloader));
            ON_CALL(*this, createProcessControllerServerSocket).WillByDefault(Return(m_processControlServerSocket));
            ON_CALL(*this, createSafeStoreRescanWorker).WillByDefault(Return(m_safeStoreRescanWorker));
            ON_CALL(*this, createMetadataRescanServerSocket).WillByDefault(Return(m_mockMetadataRescanServerSocket));
        }

        MOCK_METHOD(Common::SystemCallWrapper::ISystemCallWrapperSharedPtr , createSystemCallWrapper, (), (override));
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createSigTermHandler, (bool), (override));
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createUsr1Monitor, (common::signals::IReloadablePtr), (override));
        MOCK_METHOD(std::shared_ptr<common::signals::IReloadable>, createReloader, (threat_scanner::IThreatScannerFactorySharedPtr), (override));

        MOCK_METHOD(common::IPidLockFileSharedPtr, createPidLockFile, (const std::string& _path), (override));
        MOCK_METHOD(ISafeStoreRescanWorkerPtr, createSafeStoreRescanWorker, (const sophos_filesystem::path& safeStoreRescanSocket), (override));

        MOCK_METHOD(threat_scanner::IThreatReporterSharedPtr, createThreatReporter, (const sophos_filesystem::path socketPath), (override));
        MOCK_METHOD(threat_scanner::IScanNotificationSharedPtr , createShutdownTimer, (const sophos_filesystem::path configPath), (override));
        MOCK_METHOD(unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr, createUpdateCompleteNotifier,
                    (const sophos_filesystem::path serverPath, mode_t mode), (override));
        MOCK_METHOD(threat_scanner::IThreatScannerFactorySharedPtr, createSusiScannerFactory,
                    (threat_scanner::IThreatReporterSharedPtr reporter,
                    threat_scanner::IScanNotificationSharedPtr shutdownTimer,
                    threat_scanner::IUpdateCompleteCallbackPtr updateCompleteCallback),
                    (override));
        MOCK_METHOD(unixsocket::ScanningServerSocketPtr, createScanningServerSocket,
                    (const std::string& path,
                     mode_t mode,
                     threat_scanner::IThreatScannerFactorySharedPtr scannerFactory),
                    (override));
        MOCK_METHOD(unixsocket::ProcessControllerServerSocketPtr, createProcessControllerServerSocket,
                    (const std::string& path,
                    mode_t mode,
                    std::shared_ptr<unixsocket::IProcessControlMessageCallback> processControlCallbacks),
                    (override));
        MOCK_METHOD(unixsocket::IProcessControlMessageCallbackPtr, createThreatDetectorCallBacks,
                    (ISophosThreatDetectorMain& threatDetectorMain),
                    (override));
        MOCK_METHOD(
            std::shared_ptr<unixsocket::MetadataRescanServerSocket>,
            createMetadataRescanServerSocket,
            (const std::string& path, mode_t mode, threat_scanner::IThreatScannerFactorySharedPtr scannerFactory),
            (override));

        void setThreatDetectorCallback(std::shared_ptr<ThreatDetectorControlCallback> callback)
        {
            m_callbacks = std::move(callback);
        }

        //This is required while there is difficult inheritance from UpdateCompleteServerSocket
        void setUpdateCompleteSocketExpectation(int expectTimes)
        {
            EXPECT_CALL(*m_mockUpdateCompleteServerSocket, updateComplete()).Times(expectTimes);
        }

    private:
        std::shared_ptr<MockSystemCallWrapper> m_mockSysCalls;
        std::shared_ptr<MockSignalHandler> m_mockSigTermHandler;
        std::shared_ptr<MockSignalHandler> m_mockUsr1Monitor;
        std::shared_ptr<MockPidLock> m_mockPidLock;
        std::shared_ptr<MockThreatReporter> m_mockThreatReporter;
        std::shared_ptr<MockShutdownTimer> m_mockShutdownTimer;
        std::shared_ptr<MockUpdateCompleteServerSocket> m_mockUpdateCompleteServerSocket;
        std::shared_ptr<MockSusiScannerFactory> m_mockSusiScannerFactory;
        std::shared_ptr<MockScanningServerSocket> m_mockScanningServerSocket;
        std::shared_ptr<MockReloader> m_reloader;
        std::shared_ptr<MockSafeStoreRescanWorker> m_safeStoreRescanWorker;
        std::shared_ptr<MockMetadataRescanServerSocket> m_mockMetadataRescanServerSocket;

        //Not Mocked
        std::shared_ptr<ThreatDetectorControlCallback> m_callbacks;
        std::shared_ptr<unixsocket::ProcessControllerServerSocket> m_processControlServerSocket;
    };
}