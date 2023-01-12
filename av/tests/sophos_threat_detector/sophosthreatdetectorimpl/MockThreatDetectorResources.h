// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define TEST_PUBLIC public

#include "MockScanningServerSocket.h"
#include "MockShutdownTimer.h"
#include "MockThreatDetectorControlCallbacks.h"
#include "MockThreatReporter.h"
#include "MockUpdateCompleteServerSocket.h"

#include "sophos_threat_detector/threat_scanner/MockSusiScannerFactory.h"
#include "sophos_threat_detector/sophosthreatdetectorimpl/ThreatDetectorResources.h"

#include <common/MockPidLock.h>
#include <common/MockSignalHandler.h>
#include <datatypes/MockSysCalls.h>
#include <unixsocket/processControllerSocket/ProcessControllerServerSocket.h>

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockThreatDetectorResources : public sspl::sophosthreatdetectorimpl::IThreatDetectorResources
    {
    public:
        MockThreatDetectorResources(const fs::path& testDirectory, std::shared_ptr<NiceMock<MockSystemCallWrapper>> mockSysCallWrapper)
        {
            m_mockSysCalls = mockSysCallWrapper;
            m_mockSigTermHandler = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockUsr1Monitor = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockPidLock = std::make_shared<NiceMock<MockPidLock>>();
            m_mockThreatReporter = std::make_shared<NiceMock<MockThreatReporter>>();
            m_mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();
            m_mockUpdateCompleteServerSocket = std::make_shared<NiceMock<MockUpdateCompleteServerSocket>>(fs::path(testDirectory / "update_socket"), 0777);
            m_mockSusiScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();
            m_mockScanningServerSocket = std::make_shared<NiceMock<MockScanningServerSocket>>(fs::path(testDirectory / "scanning_server_socket"), 0777, m_mockSusiScannerFactory);

            auto mockThreatDetectorControlCallbacks = std::make_shared<NiceMock<MockThreatDetectorControlCallbacks>>();
            m_processControlServerSocket = std::make_shared<unixsocket::ProcessControllerServerSocket>(fs::path(testDirectory / "process_control_socket"), 0777, mockThreatDetectorControlCallbacks);
            m_reloader = std::make_shared<sspl::sophosthreatdetectorimpl::Reloader>(m_mockSusiScannerFactory);

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
        }

        MOCK_METHOD(datatypes::ISystemCallWrapperSharedPtr, createSystemCallWrapper, (), (override));
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createSigTermHandler, (bool), (override));
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createUsr1Monitor, (common::signals::IReloadablePtr), (override));
        MOCK_METHOD(std::shared_ptr<sspl::sophosthreatdetectorimpl::Reloader>, createReloader, (threat_scanner::IThreatScannerFactorySharedPtr), (override));

        MOCK_METHOD(common::IPidLockFileSharedPtr, createPidLockFile, (const std::string& _path), (override));
        MOCK_METHOD(threat_scanner::IThreatReporterSharedPtr, createThreatReporter, (const sophos_filesystem::path _socketPath), (override));
        MOCK_METHOD(threat_scanner::IScanNotificationSharedPtr , createShutdownTimer, (const sophos_filesystem::path _configPath), (override));
        MOCK_METHOD(unixsocket::updateCompleteSocket::UpdateCompleteServerSocketPtr, createUpdateCompleteNotifier,
                    (const sophos_filesystem::path _serverPath, mode_t _mode), (override));
        MOCK_METHOD(threat_scanner::IThreatScannerFactorySharedPtr, createSusiScannerFactory,
                    (threat_scanner::IThreatReporterSharedPtr _reporter,
                    threat_scanner::IScanNotificationSharedPtr _shutdownTimer,
                    threat_scanner::IUpdateCompleteCallbackPtr _updateCompleteCallback),
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

    private:
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCalls;
        std::shared_ptr<NiceMock<MockSignalHandler>> m_mockSigTermHandler;
        std::shared_ptr<NiceMock<MockSignalHandler>> m_mockUsr1Monitor;
        std::shared_ptr<NiceMock<MockPidLock>> m_mockPidLock;
        std::shared_ptr<NiceMock<MockThreatReporter>> m_mockThreatReporter;
        std::shared_ptr<NiceMock<MockShutdownTimer>> m_mockShutdownTimer;
        std::shared_ptr<NiceMock<MockUpdateCompleteServerSocket>> m_mockUpdateCompleteServerSocket;
        std::shared_ptr<NiceMock<MockSusiScannerFactory>> m_mockSusiScannerFactory;
        std::shared_ptr<NiceMock<MockScanningServerSocket>> m_mockScanningServerSocket;

        //Not Mocked
        std::shared_ptr<sspl::sophosthreatdetectorimpl::Reloader> m_reloader;
        std::shared_ptr<unixsocket::ProcessControllerServerSocket> m_processControlServerSocket;
    };
}