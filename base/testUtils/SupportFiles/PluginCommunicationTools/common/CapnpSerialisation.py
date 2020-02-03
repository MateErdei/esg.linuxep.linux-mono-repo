#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



import sys
import os
import capnp

from .SetupLogger import setup_logging
SYSTEMPRODUCTTESTOUTPUT = os.path.join(os.path.dirname(__file__), "..", "..", "..", "SystemProductTestOutput")
if SYSTEMPRODUCTTESTOUTPUT not in sys.path:
    sys.path.append(SYSTEMPRODUCTTESTOUTPUT)

# The following imports are made from the SystemProductTestOutput folder which is generated
# during the initialisation of the robot tests. If you wish to use this in isolation you will need
# to copy the SystemProductTestOutput folder from a build of base into the root of the System Product Tests
# See libs/SystemProductTestOutputInstall.py for more information.
import Credentials_capnp
import Process_capnp

# TimeStamp Converter
def convert_linux_epoch_to_win32_epoch(linux_epoch_time):
    wintick = 10000000
    secToEpoch = 11644473600*wintick
    return int(linux_epoch_time*wintick+secToEpoch)


# List of possible pub/swub channels for capn messages
PortScanningEventChannel = "Detector.PortScanning"
CredentialEventChannel = "Detector.Credentials"
ProcessEventChannel = "Detector.Process"
AnyDetectorChannel = "Detector"


class CredentialWrapper(object):
    def __init__(self):
        self.credentials = Credentials_capnp.CredentialsEvent.new_message()
        self.sessionTypeSchema = Credentials_capnp.CredentialsEvent.SessionType.schema.enumerants
        self.eventTypeSchema = Credentials_capnp.CredentialsEvent.EventType.schema.enumerants
        self.logger = setup_logging("CredentialWrapper", "Credential Wrapper")

    def get_message_event(self):
        return self.credentials

    def getSessionType(self):
        return str(self.credentials.sessionType)

    def getEventType(self):
        return str(self.credentials.eventType)

    def getSubjectUserSid(self):
        return {"username": str(self.credentials.subjectUserSID.username),
                "domain": str(self.credentials.subjectUserSID.domain),
                "sid": str(self.credentials.subjectUserSID.sid)}

    def getTargetUserSid(self):
        return {"username": str(self.credentials.targetUserSID.username),
                "domain": str(self.credentials.targetUserSID.domain),
                "sid": str(self.credentials.targetUserSID.sid)}

    def getGroupId(self):
        return self.credentials.userGroupID

    def getGroupName(self):
        return self.credentials.userGroupName

    def getTimestamp(self):
        return self.credentials.timestamp

    def getLogonId(self):
        return self.credentials.logonID

    def getRemoteNetworkAccess(self):
        return self.credentials.remoteNetworkAddress.address

    def setSessionType(self, sessionType):
        for key, value in self.sessionTypeSchema.items():
            if key == sessionType:
                self.credentials.sessionType = sessionType
                return
            if str(value) == sessionType:
                self.credentials.sessionType = int(sessionType)
                return
        raise AssertionError("input \"{}\" does not exist as key or value in sessionType's enumerator dictionary: {}".format(sessionType, self.sessionTypeSchema))

    def setEventType(self, eventType):
        for key, value in self.eventTypeSchema.items():
            if key == eventType:
                self.credentials.eventType = eventType
                return
            if str(value) == eventType:
                self.credentials.eventType = int(eventType)
                return
        raise AssertionError("input \"{}\" does not exist as key or value in eventType's schema enumerator dictionary: {}".format(eventType, self.eventTypeSchema))

    def setSubjectUserSid(self, username, domain, sid):
        self.credentials.subjectUserSID.username = username
        self.credentials.subjectUserSID.domain = domain
        self.credentials.subjectUserSID.sid = sid

    def setTargetUserSid(self, username, domain, sid):
        self.credentials.targetUserSID.username = username
        self.credentials.targetUserSID.domain = domain
        self.credentials.targetUserSID.sid = sid

    def setGroupId(self, userGroupID):
        try:
            self.credentials.userGroupID = int(userGroupID)
        except ValueError:
            raise AssertionError("userGroupID is not a valid integer : {}".format(userGroupID))

    def setGroupName(self, userGroupName):
        self.credentials.userGroupName = userGroupName

    def setTimestamp(self, timestamp):
        try:
            self.credentials.timestamp = int(timestamp)
        except ValueError:
            raise AssertionError("Timestamp is not a valid integer : {}".format(timestamp))

    def setLogonId(self, logonID):
        self.credentials.logonID = logonID

    def setRemoteNetworkAccess(self, remoteNetworkAddress):
        self.credentials.remoteNetworkAddress.address = remoteNetworkAddress

    def deserialise(self, byte_string):
        self.credentials = Credentials_capnp.CredentialsEvent.from_bytes(byte_string).as_builder()

    def serialise(self):
        return self.credentials.to_bytes()

class ProcessWrapper(object):
    def __init__(self):
        self.process = Process_capnp.ProcessEvent.new_message()
        self.event_type_schema = Process_capnp.ProcessEvent.EventType.schema.enumerants
        self.logger = setup_logging("ProcessWrapper", "Process Wrapper")

    def get_message_event(self):
        return self.process

    def getEventType(self):
        return str(self.process.eventType)

    def getSophosPid(self):
        return {"osPID": str(self.process.sophosPID.osPID),
                "createTime":str(self.process.sophosPID.createTime)}

    def getParentSophosPid(self):
        return {"osPID": str(self.process.parentSophosPID.osPID),
                "createTime":str(self.process.parentSophosPID.createTime)}

    def getParentSophosTid(self):
        return {"osTID": str(self.process.sophosTID.osTID),
                "createTime": str(self.process.sophosTID.createTime)}

    def getEventTypeId(self):
        return str(self.process.eventType)

    def getEndTime(self):
        return self.process.endTime

    def getFileSize(self):
        return self.process.fileSize

    def getFlags(self):
        return self.process.flags

    def getSessionId(self):
        return self.process.sessionID

    def getSid(self):
        return self.process.sid

    def getOwnerUserSid(self):
        return self.process.ownerUserSID

    def getPathname(self):
        return self.process.pathname

    def getCmdLine(self):
        return self.process.cmdLine

    def getProcTitle(self):
        return self.process.procTitle

    def getSha256(self):
        return self.process.sha256

    def getSha1(self):
        return self.process.sha1

    def setEventType(self, eventType):
        for key, value in self.event_type_schema.items():
            if key == eventType:
                self.process.eventType = eventType
                return
            if str(value) == eventType:
                self.process.eventType = int(eventType)
                return
        raise AssertionError("input \"{}\" does not exist as key or value in eventType's schema enumerator dictionary: {}".format(eventType, self.event_type_schema))

    def setSophosPid(self, osPID, createTime):
        self.process.sophosPID.osPID = osPID
        self.process.sophosPID.createTime = createTime

    def setParentSophosPid(self, osPID, createTime):
        self.process.parentSophosPID.osPID = osPID
        self.process.parentSophosPID.createTime = createTime

    def setParentSophosTid(self, osTID, createTime):
        self.process.parentSophosTID.osTID = osTID
        self.process.parentSophosTID.createTime = createTime

    def setEndTime(self, endTime):
        self.process.endTime = endTime

    def setFileSize(self, fileSize):
        self.process.fileSize.value = fileSize

    def setFlags(self, flags):
        self.process.flags = flags

    def setSessionId(self, sessionId):
        self.process.sessionId = sessionId

    def setSid(self, sid):
        self.process.sid = sid

    def setOwnerUserSid(self, name, sid, domain):
        self.process.ownerUserSID.username = name
        self.process.ownerUserSID.sid = sid
        self.process.ownerUserSID.domain = domain

    def setPathname(self,
                    flags,
                    fileSystemType,
                    driveLetter,
                    pathname,
                    openName,
                    volumeName,
                    shareName,
                    extensionName,
                    streamName,
                    finalComponentName,
                    parentDirName
    ):
        self.process.pathname.flags = flags
        self.process.pathname.fileSystemType = fileSystemType
        self.process.pathname.driveLetter = driveLetter
        self.process.pathname.pathname = pathname
        self.process.pathname.openName.length = openName[0]
        self.process.pathname.openName.offset = openName[1]
        self.process.pathname.volumeName.length = volumeName[0]
        self.process.pathname.volumeName.offset = volumeName[1]
        self.process.pathname.shareName.length = shareName[0]
        self.process.pathname.shareName.offset = shareName[1]
        self.process.pathname.extensionName.length = extensionName[0]
        self.process.pathname.extensionName.offset = extensionName[1]
        self.process.pathname.streamName.length = streamName[0]
        self.process.pathname.streamName.offset = streamName[1]
        self.process.pathname.finalComponentName.length = finalComponentName[0]
        self.process.pathname.finalComponentName.offset = finalComponentName[1]
        self.process.pathname.parentDirName.length = parentDirName[0]
        self.process.pathname.parentDirName.offset = parentDirName[1]
        

    def setCmdLine(self, cmdLine):
        self.process.cmdLine = cmdLine

    def setProcTitle(self, proc_title):
        self.process.procTitle = proc_title

    def setSha256(self, sha256):
        self.process.sha256 = sha256

    def setSha1(self, sha1):
        self.process.sha1 = sha1

    def deserialise(self, byte_string):
        self.process = Process_capnp.ProcessEvent.from_bytes(byte_string).as_builder()

    def serialise(self):
        return self.process.to_bytes()
