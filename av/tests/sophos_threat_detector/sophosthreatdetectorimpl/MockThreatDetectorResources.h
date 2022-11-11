// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

#define TEST_PUBLIC public

#include "MockShutdownTimer.h"
#include "MockThreatReporter.h"
#include "MockUpdateCompleteServerSocket.h"

#include "sophos_threat_detector/threat_scanner/MockSusiScannerFactory.h"

#include <datatypes/MockSysCalls.h>
#include <common/MockPidLock.h>
#include <common/MockSignalHandler.h>
#include <sophos_threat_detector/sophosthreatdetectorimpl/ThreatDetectorResources.h>

#include <gmock/gmock.h>

using namespace testing;

namespace
{
    class MockThreatDetectorResources : public sspl::sophosthreatdetectorimpl::IThreatDetectorResources
    {
    public:
        MockThreatDetectorResources(const fs::path& testUpdateSocketPath)
        {
            m_mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
            m_mockSigHandler = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockPidLock = std::make_shared<NiceMock<MockPidLock>>();
            m_mockThreatReporter = std::make_shared<NiceMock<MockThreatReporter>>();
            m_mockShutdownTimer = std::make_shared<NiceMock<MockShutdownTimer>>();
            m_mockUpdateCompleteServerSocket = std::make_shared<NiceMock<MockUpdateCompleteServerSocket>>(testUpdateSocketPath, 0777);
            m_mockSusiScannerFactory = std::make_shared<NiceMock<MockSusiScannerFactory>>();


            ON_CALL(*this, createSystemCallWrapper).WillByDefault(Return(m_mockSysCalls));
            ON_CALL(*this, createSignalHandler).WillByDefault(Return(m_mockSigHandler));
            ON_CALL(*this, createPidLockFile).WillByDefault(Return(m_mockPidLock));
            ON_CALL(*this, createThreatReporter).WillByDefault(Return(m_mockThreatReporter));
            ON_CALL(*this, createShutdownTimer).WillByDefault(Return(m_mockShutdownTimer));
            ON_CALL(*this, createUpdateCompleteNotifier).WillByDefault(Return(m_mockUpdateCompleteServerSocket));
            ON_CALL(*this, createSusiScannerFactory).WillByDefault(Return(m_mockSusiScannerFactory));
        }

        MOCK_METHOD(datatypes::ISystemCallWrapperSharedPtr, createSystemCallWrapper, (), (override));
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createSignalHandler, (bool), (override));
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

    private:
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCalls;
        std::shared_ptr<NiceMock<MockSignalHandler>> m_mockSigHandler;
        std::shared_ptr<NiceMock<MockPidLock>> m_mockPidLock;
        std::shared_ptr<NiceMock<MockThreatReporter>> m_mockThreatReporter;
        std::shared_ptr<NiceMock<MockShutdownTimer>> m_mockShutdownTimer;
        std::shared_ptr<NiceMock<MockUpdateCompleteServerSocket>> m_mockUpdateCompleteServerSocket;
        std::shared_ptr<NiceMock<MockSusiScannerFactory>> m_mockSusiScannerFactory;


    };
}