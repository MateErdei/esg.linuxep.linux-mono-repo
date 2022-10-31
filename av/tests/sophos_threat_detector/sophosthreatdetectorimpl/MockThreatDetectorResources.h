// Copyright 2022, Sophos Limited.  All rights reserved.

#pragma once

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
        MockThreatDetectorResources()
        {
            m_mockSysCalls = std::make_shared<NiceMock<MockSystemCallWrapper>>();
            m_mockSigHandler = std::make_shared<NiceMock<MockSignalHandler>>();
            m_mockPidLock = std::make_shared<NiceMock<MockPidLock>>();

            ON_CALL(*this, createSystemCallWrapper).WillByDefault(Return(m_mockSysCalls));
            ON_CALL(*this, createSignalHandler).WillByDefault(Return(m_mockSigHandler));
            ON_CALL(*this, createPidLockFile).WillByDefault(Return(m_mockPidLock));
        }

        MOCK_METHOD(datatypes::ISystemCallWrapperSharedPtr, createSystemCallWrapper, ());
        MOCK_METHOD(common::signals::ISignalHandlerSharedPtr, createSignalHandler, (bool));
        MOCK_METHOD(common::IPidLockFileSharedPtr, createPidLockFile, (const std::string& _path));
        MOCK_METHOD(threat_scanner::IThreatReporterSharedPtr, createThreatReporter, (const sophos_filesystem::path _socketPath));

    private:
        std::shared_ptr<NiceMock<MockSystemCallWrapper>> m_mockSysCalls;
        std::shared_ptr<NiceMock<MockSignalHandler>> m_mockSigHandler;
        std::shared_ptr<NiceMock<MockPidLock>> m_mockPidLock;
    };
}
/*

ACTION_P2(pollReturnsWithRevents, index, revents) { arg0[index].revents = revents; return 1; }
ACTION_P(readReturnsStruct, data) { *static_cast<data_type *>(arg1) = data; return sizeof(data); }
ACTION_P(readlinkReturnPath, path) { strncpy(arg1, path, arg2); return strnlen(arg1, arg2 - 1) + 1; }
*/
