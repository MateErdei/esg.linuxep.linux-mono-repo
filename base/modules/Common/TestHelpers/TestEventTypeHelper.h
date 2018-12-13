///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////

#pragma once

#include "gtest/gtest.h"
#include "TempDir.h"

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/TestHelpers/MockPathManager.h>

namespace Tests
{

    using namespace Common::EventTypes;

    class TestEventTypeHelper : public ::testing::Test
    {
    public:
        CredentialEvent createDefaultCredentialEvent()
        {
            CredentialEvent event;
            event.setEventType(Common::EventTypes::CredentialEvent::EventType::authFailure);
            event.setSessionType(Common::EventTypes::CredentialEvent::SessionType::interactive);
            event.setLogonId(1000);
            event.setTimestamp(123123123);
            event.setGroupId(1001);
            event.setGroupName("TestGroup");

            Common::EventTypes::UserSid subjectUserId;
            subjectUserId.username = "TestSubUser";
            subjectUserId.domain = "sophos.com";
            event.setSubjectUserSid(subjectUserId);

            Common::EventTypes::UserSid targetUserId;
            targetUserId.username = "TestTarUser";
            targetUserId.domain = "sophosTarget.com";
            event.setTargetUserSid(targetUserId);

            Common::EventTypes::NetworkAddress network;
            network.address = "sophos.com:400";
            event.setRemoteNetworkAccess(network);

            return event;
        }


        ::testing::AssertionResult credentialEventIsEquivalent( const char* m_expr,
                                                                const char* n_expr,
                                                                const Common::EventTypes::CredentialEvent expected,
                                                                const Common::EventTypes::CredentialEvent resulted)
        {
            std::stringstream s;
            s<< m_expr << " and " << n_expr << " failed: ";

            if ( expected.getSessionType() != resulted.getSessionType())
            {
                return ::testing::AssertionFailure() << s.str() << "SessionType differs";
            }

            if ( expected.getEventType() != resulted.getEventType())
            {
                return ::testing::AssertionFailure() << s.str() << "EventType differs";
            }

            if (expected.getEventTypeId() != resulted.getEventTypeId())
            {
                return ::testing::AssertionFailure() << s.str() << "EventTypeId differs";
            }

            if (expected.getSubjectUserSid().domain != resulted.getSubjectUserSid().domain)
            {
                return ::testing::AssertionFailure() << s.str() << "SubjectUserSid domain differs";
            }

            if (expected.getSubjectUserSid().username != resulted.getSubjectUserSid().username)
            {
                return ::testing::AssertionFailure() << s.str() << "SubjectUserSid username differs";
            }

            if (expected.getTargetUserSid().domain != resulted.getTargetUserSid().domain)
            {
                return ::testing::AssertionFailure() << s.str() << "TargetUserSid domain differs";
            }

            if (expected.getTargetUserSid().username != resulted.getTargetUserSid().username)
            {
                return ::testing::AssertionFailure() << s.str() << "TargetUserSid username differs";
            }

            if (expected.getGroupId() != resulted.getGroupId())
            {
                return ::testing::AssertionFailure() << s.str() << "GroupId differs";
            }

            if (expected.getGroupName() != resulted.getGroupName())
            {
                return ::testing::AssertionFailure() << s.str() << "GroupName differs";
            }

            if (expected.getTimestamp() != resulted.getTimestamp())
            {
                return ::testing::AssertionFailure() << s.str() << "Timestamp differs";
            }

            if (expected.getLogonId() != resulted.getLogonId())
            {
                return ::testing::AssertionFailure() << s.str() << "logonId differs";
            }

            if (expected.getRemoteNetworkAccess().address != resulted.getRemoteNetworkAccess().address)
            {
                return ::testing::AssertionFailure() << s.str() << "RemoteNetworkAccess address differs";
            }

            return ::testing::AssertionSuccess();
        }

        PortScanningEvent createDefaultPortScanningEvent()
        {
            PortScanningEvent event;

            auto connection = event.getConnection();
            connection.sourceAddress.address = "192.168.0.1";
            connection.sourceAddress.port = 80;
            connection.destinationAddress.address = "192.168.0.2";
            connection.destinationAddress.port = 80;
            connection.protocol = 25;
            event.setConnection(connection);
            event.setEventType(Common::EventTypes::PortScanningEvent::opened);
            return event;
        }

        Common::EventTypes::IpFlow createDefaultIpFlow()
        {
            Common::EventTypes::IpFlow ipFlow;
            ipFlow.destinationAddress.address="182.158";
            ipFlow.destinationAddress.port=90;
            ipFlow.sourceAddress.address="182.136";
            ipFlow.sourceAddress.port=800;
            ipFlow.protocol=2;
            return ipFlow;
        }

        ::testing::AssertionResult portScanningEventIsEquivalent( const char* m_expr,
                                                                  const char* n_expr,
                                                                  const Common::EventTypes::PortScanningEvent expected,
                                                                  const Common::EventTypes::PortScanningEvent resulted)
        {
            std::stringstream s;
            s<< m_expr << " and " << n_expr << " failed: ";


            if ( expected.getConnection().sourceAddress.address != resulted.getConnection().sourceAddress.address )
            {
                return ::testing::AssertionFailure() << s.str() << "Connection source address differs";
            }

            if ( expected.getConnection().sourceAddress.port != resulted.getConnection().sourceAddress.port )
            {
                return ::testing::AssertionFailure() << s.str() << "Connection source port differs";
            }

            if ( expected.getConnection().destinationAddress.address != resulted.getConnection().destinationAddress.address )
            {
                return ::testing::AssertionFailure() << s.str() << "Connection destination address differs";
            }

            if ( expected.getConnection().destinationAddress.port != resulted.getConnection().destinationAddress.port )
            {
                return ::testing::AssertionFailure() << s.str() << "Connection destination port differs";
            }

            if ( expected.getConnection().protocol != resulted.getConnection().protocol )
            {
                return ::testing::AssertionFailure() << s.str() << "Connection protocal differs";
            }

            if ( expected.getEventType() != resulted.getEventType() )
            {
                return ::testing::AssertionFailure() << s.str() << "Event type differs";
            }

            return ::testing::AssertionSuccess();
        }

        void setUpApplicationManager()
        {
            MockedApplicationPathManager *mockAppManager = new NiceMock<MockedApplicationPathManager>();
            MockedApplicationPathManager &mock(*mockAppManager);
            ON_CALL(mock, getPublisherDataChannelAddress()).WillByDefault(Return("inproc://datachannel.ipc"));
            ON_CALL(mock, getSubscriberDataChannelAddress()).WillByDefault(Return("inproc://datachannel.ipc"));
            Common::ApplicationConfiguration::replaceApplicationPathManager(
                    std::unique_ptr<Common::ApplicationConfiguration::IApplicationPathManager>(mockAppManager));
        }

        void ApplicationManagerTearDown()
        {
            Common::ApplicationConfiguration::restoreApplicationPathManager();
        }


    };

}
