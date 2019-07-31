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
            subjectUserId.machineid = "123456789";
            subjectUserId.userid = 1000;
            event.setSubjectUserSid(subjectUserId);

            Common::EventTypes::UserSid targetUserId;
            targetUserId.username = "TestTarUser";
            targetUserId.domain = "sophosTarget.com";
            targetUserId.machineid = "123456789";
            targetUserId.userid = 1001;
            event.setTargetUserSid(targetUserId);

            Common::EventTypes::NetworkAddress network;
            network.address = "sophos.com:400";
            event.setRemoteNetworkAccess(network);

            Common::EventTypes::SophosPid sophosPid;
            sophosPid.timestamp = 123123123;
            sophosPid.pid = 552211;

            event.setSophosPid(sophosPid);

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
                return ::testing::AssertionFailure()
                       << s.str() << "SessionType differs: Expected = " << expected.getSessionType()
                       << " , Actual = " << resulted.getSessionType();
            }

            if (expected.getEventType() != resulted.getEventType())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "EventType differs: Expected = " << expected.getEventType()
                       << " , Actual = " << resulted.getEventType();
            }

            if (expected.getEventTypeId() != resulted.getEventTypeId())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "EventTypeId differs: Expected = " << expected.getEventTypeId()
                       << " , Actual = " << resulted.getEventTypeId();
            }

            if (expected.getSubjectUserSid().domain != resulted.getSubjectUserSid().domain)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SubjectUserSid domain differs: Expected = " << expected.getSubjectUserSid().domain
                       << " , Actual = " << resulted.getSubjectUserSid().domain;
            }

            if (expected.getSubjectUserSid().username != resulted.getSubjectUserSid().username)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "SubjectUserSid username differs: Expected = " << expected.getSubjectUserSid().username
                       << " , Actual = " << resulted.getSubjectUserSid().username;
            }

            if (expected.getSubjectUserSid().machineid != resulted.getSubjectUserSid().machineid)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "SubjectUserSid machineid differs: Expected = " << expected.getSubjectUserSid().machineid
                       << " , Actual = " << resulted.getSubjectUserSid().machineid;
            }

            if (expected.getSubjectUserSid().userid != resulted.getSubjectUserSid().userid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SubjectUserSid userid differs: Expected = " << expected.getSubjectUserSid().userid
                       << " , Actual = " << resulted.getSubjectUserSid().userid;
            }

            if (expected.getTargetUserSid().domain != resulted.getTargetUserSid().domain)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "TargetUserSid domain differs: Expected = " << expected.getTargetUserSid().domain
                       << " , Actual = " << resulted.getTargetUserSid().domain;
            }

            if (expected.getTargetUserSid().username != resulted.getTargetUserSid().username)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "TargetUserSid username differs: Expected = " << expected.getTargetUserSid().username
                       << " , Actual = " << resulted.getTargetUserSid().username;
            }

            if (expected.getTargetUserSid().machineid != resulted.getTargetUserSid().machineid)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "TargetUserSid machineid differs: Expected = " << expected.getTargetUserSid().machineid
                       << " , Actual = " << resulted.getTargetUserSid().machineid;
            }

            if (expected.getTargetUserSid().userid != resulted.getTargetUserSid().userid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "TargetUserSid userid differs: Expected = " << expected.getTargetUserSid().userid
                       << " , Actual = " << resulted.getTargetUserSid().userid;
            }

            if (expected.getGroupId() != resulted.getGroupId())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "GroupId differs: Expected = " << expected.getGroupId()
                       << " , Actual = " << resulted.getGroupId();
            }

            if (expected.getGroupName() != resulted.getGroupName())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "GroupName differs: Expected = " << expected.getGroupName()
                       << " , Actual = " << resulted.getGroupName();
            }

            if (expected.getTimestamp() != resulted.getTimestamp())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Timestamp differs: Expected = " << expected.getTimestamp()
                       << " , Actual = " << resulted.getTimestamp();
            }

            if (expected.getLogonId() != resulted.getLogonId())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "logonId differs: Expected = " << expected.getLogonId()
                       << " , Actual = " << resulted.getLogonId();
            }

            if (expected.getRemoteNetworkAccess().address != resulted.getRemoteNetworkAccess().address)
            {
                return ::testing::AssertionFailure() << s.str() << "RemoteNetworkAccess address differs: Expected = "
                                                     << expected.getRemoteNetworkAccess().address
                                                     << " , Actual = " << resulted.getRemoteNetworkAccess().address;
            }

            if (expected.getSophosPid().timestamp != resulted.getSophosPid().timestamp)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SophosPid timestamp differs: Expected = " << expected.getSophosPid().timestamp
                       << " , Actual = " << resulted.getSophosPid().timestamp;
            }

            if (expected.getSophosPid().pid != resulted.getSophosPid().pid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SophosPid pid differs: Expected = " << expected.getSophosPid().pid
                       << " , Actual = " << resulted.getSophosPid().pid;
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
            userSid.machineid = "1234567890";
            userSid.userid = 1001;
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
            const Common::EventTypes::ProcessEvent& expected,
            const Common::EventTypes::ProcessEvent& resulted)
        {
            std::stringstream s;
            s << m_expr << " and " << n_expr << " failed: ";

            if (expected.getEventType() != resulted.getEventType())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "EventType differs: Expected = " << expected.getEventType()
                       << " , Actual = " << resulted.getEventType();
            }

            if (expected.getEventTypeId() != resulted.getEventTypeId())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "EventTypeId differs: Expected = " << expected.getEventTypeId()
                       << " , Actual = " << resulted.getEventTypeId();
            }

            Common::EventTypes::SophosPid expectedSophosPid = expected.getSophosPid();
            Common::EventTypes::SophosPid resultedSophosPid = resulted.getSophosPid();

            if (expectedSophosPid.pid != resultedSophosPid.pid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SophosPID PID differs: Expected = " << expectedSophosPid.pid
                       << " , Actual = " << resultedSophosPid.pid;
            }

            if (expectedSophosPid.timestamp != resultedSophosPid.timestamp)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SophosPID Timestamp differs: Expected = " << expectedSophosPid.timestamp
                       << " , Actual = " << resultedSophosPid.timestamp;
            }

            Common::EventTypes::SophosPid expectedParentSophosPid = expected.getParentSophosPid();
            Common::EventTypes::SophosPid resultedParentSophosPid = resulted.getParentSophosPid();

            if (expectedParentSophosPid.pid != resultedParentSophosPid.pid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "ParentSophosPID PID differs: Expected = " << expectedParentSophosPid.pid
                       << " , Actual = " << resultedParentSophosPid.pid;
            }

            if (expectedParentSophosPid.timestamp != resultedParentSophosPid.timestamp)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "ParentSophosPID Timestamp differs: Expected = " << expectedParentSophosPid.timestamp
                       << " , Actual = " << resultedParentSophosPid.timestamp;
            }

            Common::EventTypes::SophosTid expectedParentSophosTid = expected.getParentSophosTid();
            Common::EventTypes::SophosTid resultedParentSophosTid = resulted.getParentSophosTid();

            if (expectedParentSophosTid.tid != resultedParentSophosTid.tid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "ParentSophosTID TID differs: Expected = " << expectedParentSophosTid.tid
                       << " , Actual = " << resultedParentSophosTid.tid;
            }

            if (expectedParentSophosTid.timestamp != resultedParentSophosTid.timestamp)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "ParentSophosTID Timestamp differs: Expected = " << expectedParentSophosTid.timestamp
                       << " , Actual = " << resultedParentSophosTid.timestamp;
            }

            if (expected.getEndTime() != resulted.getEndTime())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "EndTime differs: Expected = " << expected.getEndTime()
                       << " , Actual = " << resulted.getEndTime();
            }

            if (expected.getFileSize().value != resulted.getFileSize().value)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "FileSize differs: Expected = " << expected.getFileSize().value
                       << " , Actual = " << resulted.getFileSize().value;
            }

            if (expected.getFlags() != resulted.getFlags())
            {
                return ::testing::AssertionFailure() << s.str() << "Flags differ: Expected = " << expected.getFlags()
                                                     << " , Actual = " << resulted.getFlags();
            }

            if (expected.getSessionId() != resulted.getSessionId())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "SessionId differ: Expected = " << expected.getSessionId()
                       << " , Actual = " << resulted.getSessionId();
            }

            if (expected.getSid() != resulted.getSid())
            {
                return ::testing::AssertionFailure() << s.str() << "SID differ: Expected = " << expected.getSid()
                                                     << " , Actual = " << resulted.getSid();
            }

            if (expected.getOwnerUserSid().username != resulted.getOwnerUserSid().username)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "Owner User Sid - username differ: Expected = " << expected.getOwnerUserSid().username
                       << " , Actual = " << resulted.getOwnerUserSid().username;
            }

            if (expected.getOwnerUserSid().domain != resulted.getOwnerUserSid().domain)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Owner User Sid - domain differ: Expected = " << expected.getOwnerUserSid().domain
                       << " , Actual = " << resulted.getOwnerUserSid().domain;
            }

            if (expected.getOwnerUserSid().sid != resulted.getOwnerUserSid().sid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Owner User Sid - sid differ: Expected = " << expected.getOwnerUserSid().sid
                       << " , Actual = " << resulted.getOwnerUserSid().sid;
            }

            if (expected.getOwnerUserSid().machineid != resulted.getOwnerUserSid().machineid)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "Owner User Sid - machineid differ: Expected = " << expected.getOwnerUserSid().machineid
                       << " , Actual = " << resulted.getOwnerUserSid().machineid;
            }

            if (expected.getOwnerUserSid().userid != resulted.getOwnerUserSid().userid)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Owner User Sid - userid differ: Expected = " << expected.getOwnerUserSid().userid
                       << " , Actual = " << resulted.getOwnerUserSid().userid;
            }

            Common::EventTypes::Pathname expectedPathname = expected.getPathname();
            Common::EventTypes::Pathname resultedPathname = resulted.getPathname();

            if (expectedPathname.flags != resultedPathname.flags)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName Flags differ: Expected = " << expectedPathname.flags
                       << " , Actual = " << resultedPathname.flags;
            }

            if (expectedPathname.fileSystemType != resultedPathname.fileSystemType)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName FileSystemType differs: Expected = " << expectedPathname.fileSystemType
                       << " , Actual = " << resultedPathname.fileSystemType;
            }

            if (expectedPathname.driveLetter != resultedPathname.driveLetter)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName DriveLetter differs: Expected = " << expectedPathname.driveLetter
                       << " , Actual = " << resultedPathname.driveLetter;
            }

            if (expectedPathname.pathname != resultedPathname.pathname)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName PathName differs: Expected = " << expectedPathname.pathname
                       << " , Actual = " << resultedPathname.pathname;
            }

            if (expectedPathname.openName.offset != resultedPathname.openName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName OpenName Offset differs: Expected = " << expectedPathname.openName.offset
                       << " , Actual = " << resultedPathname.openName.offset;
            }

            if (expectedPathname.openName.length != resultedPathname.openName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName OpenName Length differs: Expected = " << expectedPathname.openName.length
                       << " , Actual = " << resultedPathname.openName.length;
            }

            if (expectedPathname.volumeName.length != resultedPathname.volumeName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName VolumeName Length differs: Expected = " << expectedPathname.volumeName.length
                       << " , Actual = " << resultedPathname.volumeName.length;
            }

            if (expectedPathname.volumeName.offset != resultedPathname.volumeName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName VolumeName Offset differs: Expected = " << expectedPathname.volumeName.offset
                       << " , Actual = " << resultedPathname.volumeName.offset;
            }

            if (expectedPathname.shareName.length != resultedPathname.shareName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ShareName Length differs: Expected = " << expectedPathname.shareName.length
                       << " , Actual = " << resultedPathname.shareName.length;
            }

            if (expectedPathname.shareName.offset != resultedPathname.shareName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ShareName Offset differs: Expected = " << expectedPathname.shareName.offset
                       << " , Actual = " << resultedPathname.shareName.offset;
            }

            if (expectedPathname.extensionName.length != resultedPathname.extensionName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ExtensionName Length differs: Expected = " << expectedPathname.extensionName.length
                       << " , Actual = " << resultedPathname.extensionName.length;
            }

            if (expectedPathname.extensionName.offset != resultedPathname.extensionName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ExtensionName Offset differs: Expected = " << expectedPathname.extensionName.offset
                       << " , Actual = " << resultedPathname.extensionName.offset;
            }

            if (expectedPathname.streamName.length != resultedPathname.streamName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName StreamName Length differs: Expected = " << expectedPathname.streamName.length
                       << " , Actual = " << resultedPathname.streamName.length;
            }

            if (expectedPathname.streamName.offset != resultedPathname.streamName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName StreamName Offset differs: Expected = " << expectedPathname.streamName.offset
                       << " , Actual = " << resultedPathname.streamName.offset;
            }

            if (expectedPathname.finalComponentName.length != resultedPathname.finalComponentName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName FinalComponentName Length differs: Expected = "
                       << expectedPathname.finalComponentName.length
                       << " , Actual = " << resultedPathname.finalComponentName.length;
            }

            if (expectedPathname.finalComponentName.offset != resultedPathname.finalComponentName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "PathName FinalComponentName Offset differs: Expected = "
                       << expectedPathname.finalComponentName.offset
                       << " , Actual = " << resultedPathname.finalComponentName.offset;
            }

            if (expectedPathname.parentDirName.length != resultedPathname.parentDirName.length)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ParentDirName Length differs: Expected = " << expectedPathname.parentDirName.length
                       << " , Actual = " << resultedPathname.parentDirName.length;
            }

            if (expectedPathname.parentDirName.offset != resultedPathname.parentDirName.offset)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "PathName ParentDirName Offset differs: Expected = " << expectedPathname.parentDirName.offset
                       << " , Actual = " << resultedPathname.parentDirName.offset;
            }

            if (expected.getCmdLine() != resulted.getCmdLine())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "CMDLine differs: Expected = " << expected.getCmdLine()
                       << " , Actual = " << resulted.getCmdLine();
            }

            if (expected.getSha256() != resulted.getSha256())
            {
                return ::testing::AssertionFailure() << s.str() << "sha256 differs: Expected = " << expected.getSha256()
                                                     << " , Actual = " << resulted.getSha256();
            }

            if (expected.getSha1() != resulted.getSha1())
            {
                return ::testing::AssertionFailure() << s.str() << "sha1 differs: Expected = " << expected.getSha1()
                                                     << " , Actual = " << resulted.getSha1();
            }

            if (expected.getProcTitle() != resulted.getProcTitle())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "procTitle differs: Expected = " << expected.getProcTitle()
                       << " , Actual = " << resulted.getProcTitle();
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
                return ::testing::AssertionFailure()
                       << s.str() << "Connection source address differs: Expected = " << expected.getEventType()
                       << " , Actual = " << resulted.getEventType();
            }

            if (expected.getConnection().sourceAddress.port != resulted.getConnection().sourceAddress.port)
            {
                return ::testing::AssertionFailure()
                       << s.str()
                       << "Connection source port differs: Expected = " << expected.getConnection().sourceAddress.port
                       << " , Actual = " << resulted.getConnection().sourceAddress.port;
            }

            if (expected.getConnection().destinationAddress.address !=
                resulted.getConnection().destinationAddress.address)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Connection destination address differs: Expected = "
                       << expected.getConnection().destinationAddress.address
                       << " , Actual = " << resulted.getConnection().destinationAddress.address;
            }

            if (expected.getConnection().destinationAddress.port != resulted.getConnection().destinationAddress.port)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Connection destination port differs: Expected = "
                       << expected.getConnection().destinationAddress.port
                       << " , Actual = " << resulted.getConnection().destinationAddress.port;
            }

            if (expected.getConnection().protocol != resulted.getConnection().protocol)
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Connection protocal differs: Expected = " << expected.getConnection().protocol
                       << " , Actual = " << resulted.getConnection().protocol;
            }

            if (expected.getEventType() != resulted.getEventType())
            {
                return ::testing::AssertionFailure()
                       << s.str() << "Event type differs: Expected = " << expected.getEventType()
                       << " , Actual = " << resulted.getEventType();
            }

            return ::testing::AssertionSuccess();
        }
    };

} // namespace Tests
