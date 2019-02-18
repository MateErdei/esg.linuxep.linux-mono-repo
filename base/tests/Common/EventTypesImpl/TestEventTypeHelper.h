/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include "gtest/gtest.h"

#include <Common/EventTypes/CredentialEvent.h>
#include <Common/EventTypes/PortScanningEvent.h>
#include <Common/EventTypes/ProcessEvent.h>
#include <tests/Common/Helpers/TempDir.h>

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

        ::testing::AssertionResult credentialEventIsEquivalent(
            const char* m_expr,
            const char* n_expr,
            const Common::EventTypes::CredentialEvent& expected,
            const Common::EventTypes::CredentialEvent& resulted)
        {
            std::stringstream s;
            s << m_expr << " and " << n_expr << " failed: ";

            if (expected.getSessionType() != resulted.getSessionType())
            {
                return ::testing::AssertionFailure() << s.str() << "SessionType differs";
            }

            if (expected.getEventType() != resulted.getEventType())
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

        ProcessEvent createDefaultProcessEvent()
        {
            ProcessEvent event;

            event.setEventType(Common::EventTypes::ProcessEvent::EventType::end);

            Common::EventTypes::SophosPid sophosPid;
            sophosPid.pid = 10084;
            sophosPid.timestamp = 13175849586748;
            event.setSophosPid(sophosPid);

            Common::EventTypes::SophosPid parentSophosPid;
            parentSophosPid.pid = 10124;
            parentSophosPid.timestamp = 1317195849586748;
            event.setParentSophosPid(parentSophosPid);

            Common::EventTypes::SophosTid parentSophosTid;
            parentSophosTid.tid = 8124;
            parentSophosTid.timestamp = 13171163969586748;
            event.setParentSophosTid(parentSophosTid);

            event.setEndTime(1510293458);
            Common::EventTypes::OptionalUInt64 fileSize;
            fileSize.value = 123;
            event.setFileSize(fileSize);
            event.setFlags(48);
            event.setSessionId(312);

            event.setSid("1001");

            Common::EventTypes::UserSid userSid;
            userSid.username = "testUser";
            userSid.domain = "testDomain";
            userSid.sid = "1001";
            event.setOwnerUserSid(userSid);

            Common::EventTypes::Pathname pathname;
            Common::EventTypes::TextOffsetLength openName;
            Common::EventTypes::TextOffsetLength volumeName;
            Common::EventTypes::TextOffsetLength shareName;
            Common::EventTypes::TextOffsetLength extensionName;
            Common::EventTypes::TextOffsetLength streamName;
            Common::EventTypes::TextOffsetLength finalComponentName;
            Common::EventTypes::TextOffsetLength parentDirName;
            pathname.flags = 12;
            pathname.fileSystemType = 452;
            pathname.driveLetter = 6;
            pathname.pathname = "/name/of/path";
            openName.length = 23;
            openName.offset = 22;
            pathname.openName = openName;
            volumeName.length = 21;
            volumeName.offset = 20;
            pathname.volumeName = volumeName;
            shareName.length = 19;
            shareName.offset = 18;
            pathname.shareName = shareName;
            extensionName.length = 17;
            extensionName.offset = 16;
            pathname.extensionName = extensionName;
            streamName.length = 15;
            streamName.offset = 14;
            pathname.streamName = streamName;
            finalComponentName.length = 13;
            finalComponentName.offset = 12;
            pathname.finalComponentName = finalComponentName;
            parentDirName.length = 11;
            parentDirName.offset = 10;
            pathname.parentDirName = parentDirName;
            event.setPathname(pathname);

            event.setCmdLine("example cmd line");
            event.setSha256("somesha256");
            event.setSha1("somesha1");

            return event;
        }

        ::testing::AssertionResult processEventIsEquivalent(
            const char* m_expr,
            const char* n_expr,
            const Common::EventTypes::ProcessEvent expected,
            const Common::EventTypes::ProcessEvent actual)
        {
            std::stringstream s;
            s << m_expr << " and " << n_expr << " failed: ";

            if (expected.getEventType() != actual.getEventType())
            {
                return ::testing::AssertionFailure() << s.str() << "EventType differs";
            }

            if (expected.getEventTypeId() != actual.getEventTypeId())
            {
                return ::testing::AssertionFailure() << s.str() << "EventTypeId differs";
            }

            Common::EventTypes::SophosPid expectedSophosPid = expected.getSophosPid();
            Common::EventTypes::SophosPid actualSophosPid = actual.getSophosPid();

            if (expectedSophosPid.pid != actualSophosPid.pid)
            {
                return ::testing::AssertionFailure() << s.str() << "SophosPID PID differs";
            }

            if (expectedSophosPid.timestamp != actualSophosPid.timestamp)
            {
                return ::testing::AssertionFailure() << s.str() << "SophosPID Timestamp differs";
            }

            Common::EventTypes::SophosPid expectedParentSophosPid = expected.getParentSophosPid();
            Common::EventTypes::SophosPid actualParentSophosPid = actual.getParentSophosPid();

            if (expectedParentSophosPid.pid != actualParentSophosPid.pid)
            {
                return ::testing::AssertionFailure() << s.str() << "ParentSophosPID PID differs";
            }

            if (expectedParentSophosPid.timestamp != actualParentSophosPid.timestamp)
            {
                return ::testing::AssertionFailure() << s.str() << "ParentSophosPID Timestamp differs";
            }

            Common::EventTypes::SophosTid expectedParentSophosTid = expected.getParentSophosTid();
            Common::EventTypes::SophosTid actualParentSophosTid = actual.getParentSophosTid();

            if (expectedParentSophosTid.tid != actualParentSophosTid.tid)
            {
                return ::testing::AssertionFailure() << s.str() << "ParentSophosTID TID differs";
            }

            if (expectedParentSophosTid.timestamp != actualParentSophosTid.timestamp)
            {
                return ::testing::AssertionFailure() << s.str() << "ParentSophosTID Timestamp differs";
            }

            if (expected.getEndTime() != actual.getEndTime())
            {
                return ::testing::AssertionFailure() << s.str() << "EndTime differs";
            }

            if (expected.getFileSize().value != actual.getFileSize().value)
            {
                return ::testing::AssertionFailure() << s.str() << "FileSize differs";
            }

            if (expected.getFlags() != actual.getFlags())
            {
                return ::testing::AssertionFailure() << s.str() << "Flags differ";
            }

            if (expected.getSessionId() != actual.getSessionId())
            {
                return ::testing::AssertionFailure() << s.str() << "SessionId differ";
            }

            if (expected.getSid() != actual.getSid())
            {
                return ::testing::AssertionFailure() << s.str() << "SID differ";
            }

            if ((expected.getOwnerUserSid().username != actual.getOwnerUserSid().username) ||
                (expected.getOwnerUserSid().domain != actual.getOwnerUserSid().domain) ||
                (expected.getOwnerUserSid().sid != actual.getOwnerUserSid().sid))
            {
                return ::testing::AssertionFailure() << s.str() << "Owner User Sid differ";
            }

            Common::EventTypes::Pathname expectedPathname = expected.getPathname();
            Common::EventTypes::Pathname actualPathname = actual.getPathname();

            if (expectedPathname.flags != actualPathname.flags)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName Flags differ";
            }

            if (expectedPathname.fileSystemType != actualPathname.fileSystemType)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName FileSystemType differs";
            }

            if (expectedPathname.driveLetter != actualPathname.driveLetter)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName DriveLetter differs";
            }

            if (expectedPathname.pathname != actualPathname.pathname)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName PathName differs";
            }

            if (expectedPathname.openName.offset != actualPathname.openName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName OpenName Offset differs";
            }

            if (expectedPathname.openName.length != actualPathname.openName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName OpenName Length differs";
            }

            if (expectedPathname.volumeName.length != actualPathname.volumeName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName VolumeName Length differs";
            }

            if (expectedPathname.volumeName.offset != actualPathname.volumeName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName VolumeName Offset differs";
            }

            if (expectedPathname.shareName.length != actualPathname.shareName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ShareName Length differs";
            }

            if (expectedPathname.shareName.offset != actualPathname.shareName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ShareName Offset differs";
            }

            if (expectedPathname.extensionName.length != actualPathname.extensionName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ExtensionName Length differs";
            }

            if (expectedPathname.extensionName.offset != actualPathname.extensionName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ExtensionName Offset differs";
            }

            if (expectedPathname.streamName.length != actualPathname.streamName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName StreamName Length differs";
            }

            if (expectedPathname.streamName.offset != actualPathname.streamName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName StreamName Offset differs";
            }

            if (expectedPathname.finalComponentName.length != actualPathname.finalComponentName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName FinalComponentName Length differs";
            }

            if (expectedPathname.finalComponentName.offset != actualPathname.finalComponentName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName FinalComponentName Offset differs";
            }

            if (expectedPathname.parentDirName.length != actualPathname.parentDirName.length)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ParentDirName Length differs";
            }

            if (expectedPathname.parentDirName.offset != actualPathname.parentDirName.offset)
            {
                return ::testing::AssertionFailure() << s.str() << "PathName ParentDirName Offset differs";
            }

            if (expected.getCmdLine() != actual.getCmdLine())
            {
                return ::testing::AssertionFailure() << s.str() << "CMDLine differs";
            }

            if (expected.getSha256() != actual.getSha256())
            {
                return ::testing::AssertionFailure() << s.str() << "sha256 differs";
            }

            if (expected.getSha1() != actual.getSha1())
            {
                return ::testing::AssertionFailure() << s.str() << "sha1 differs";
            }

            if (expected.getProcTitle() != actual.getProcTitle())
            {
                return ::testing::AssertionFailure() << s.str() << "procTitle differs";
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
            ipFlow.destinationAddress.address = "182.158";
            ipFlow.destinationAddress.port = 90;
            ipFlow.sourceAddress.address = "182.136";
            ipFlow.sourceAddress.port = 800;
            ipFlow.protocol = 2;
            return ipFlow;
        }

        ::testing::AssertionResult portScanningEventIsEquivalent(
            const char* m_expr,
            const char* n_expr,
            const Common::EventTypes::PortScanningEvent expected,
            const Common::EventTypes::PortScanningEvent resulted)
        {
            std::stringstream s;
            s << m_expr << " and " << n_expr << " failed: ";

            if (expected.getConnection().sourceAddress.address != resulted.getConnection().sourceAddress.address)
            {
                return ::testing::AssertionFailure() << s.str() << "Connection source address differs";
            }

            if (expected.getConnection().sourceAddress.port != resulted.getConnection().sourceAddress.port)
            {
                return ::testing::AssertionFailure() << s.str() << "Connection source port differs";
            }

            if (expected.getConnection().destinationAddress.address !=
                resulted.getConnection().destinationAddress.address)
            {
                return ::testing::AssertionFailure() << s.str() << "Connection destination address differs";
            }

            if (expected.getConnection().destinationAddress.port != resulted.getConnection().destinationAddress.port)
            {
                return ::testing::AssertionFailure() << s.str() << "Connection destination port differs";
            }

            if (expected.getConnection().protocol != resulted.getConnection().protocol)
            {
                return ::testing::AssertionFailure() << s.str() << "Connection protocal differs";
            }

            if (expected.getEventType() != resulted.getEventType())
            {
                return ::testing::AssertionFailure() << s.str() << "Event type differs";
            }

            return ::testing::AssertionSuccess();
        }
    };

} // namespace Tests
