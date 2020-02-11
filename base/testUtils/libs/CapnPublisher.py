#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import time
from FakeMultiPublisher import FakeMultiPublisher

import PathManager
SUPPORTFILESPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILESPATH)


try:
    from PluginCommunicationTools.common.CapnpSerialisation import CredentialWrapper, CredentialEventChannel, ProcessWrapper, ProcessEventChannel, convert_linux_epoch_to_win32_epoch
    from PluginCommunicationTools.common.SetupLogger import setup_logging
    CAPNPENABLED=True
except Exception as ex:
    CAPNPENABLED=False
    CAPNPNEXCEPTION = ex

def require_capnpn():
    if not CAPNPENABLED:
        raise AssertionError("Capnpn not setup correctly {}".format(CAPNPNEXCEPTION))

class CapnPublisher(object):
    def __init__(self):
        self.publisher = None
        self.logger = setup_logging("CapnPublisher.log", "Capn Publisher")

    def __del__(self):
        if self.publisher:
            self.publisher = None

    def start_publisher(self):
        require_capnpn()
        self.publisher = FakeMultiPublisher()
        self.publisher.add_publishers(1)

    def create_and_send_authentication_fail_with_current_time_plus_time_seconds(self, timeOffset = "0"):
        require_capnpn()
        message = CredentialWrapper()
        message.setSessionType("network")
        message.setEventType("authFailure")
        message.setSubjectUserSid("DummyUser", "Dummy.Domain", "DummySID")
        message.setTargetUserSid("DummyUser", "Dummy.Domain", "DummySID")
        message.setGroupId(1000)
        message.setGroupName("DummyGroup")
        message.setTimestamp(convert_linux_epoch_to_win32_epoch(time.time()+int(timeOffset)))
        self.send_message(CredentialEventChannel, message.serialise())

    def create_and_send_authentication_fail_containing_network_address_with_current_time_plus_time_seconds(self, networkAddress="123.123.123.123", timeOffset = "0"):
        require_capnpn()
        message = CredentialWrapper()
        message.setSessionType("network")
        message.setEventType("authFailure")
        message.setSubjectUserSid("DummyUser", "Dummy.Domain", "DummySID")
        message.setTargetUserSid("DummyUser", "Dummy.Domain", "DummySID")
        message.setGroupId(1000)
        message.setGroupName("DummyGroup")
        message.setTimestamp(convert_linux_epoch_to_win32_epoch(time.time()+int(timeOffset)))
        message.setRemoteNetworkAccess(networkAddress)
        self.send_message(CredentialEventChannel, message.serialise())

    def create_and_send_wget_process_event(self, type="start"):
        require_capnpn()
        message = ProcessWrapper()
        message.setEventType(type)
        message.setParentSophosPid(1000, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setParentSophosTid(1500, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setSophosPid(2000, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setEndTime(convert_linux_epoch_to_win32_epoch(time.time()))
        message.setFileSize(1024)
        message.setFlags(777)
        message.setSessionId(600)
        message.setSid("1001")
        message.setOwnerUserSid("testUser", "DummySID", "testDomain")
        message.setPathname(12,
                            452,
                            6,
                            "/usr/bin/wget",
                            (0,0),
                            (0,0),
                            (0,0),
                            (0,0),
                            (0,0),
                            (4,9),
                            (8,0))
        message.setCmdLine("wget SuspiciousUrl.net")
        self.send_message(ProcessEventChannel, message.serialise())

    def create_and_send_whoami_process_event(self, type="start"):
        require_capnpn()
        message = ProcessWrapper()
        message.setEventType(type)
        message.setParentSophosPid(1000, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setParentSophosTid(1500, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setSophosPid(2000, convert_linux_epoch_to_win32_epoch(time.time()))
        message.setEndTime(convert_linux_epoch_to_win32_epoch(time.time()))
        message.setFileSize(1024)
        message.setFlags(777)
        message.setSessionId(600)
        message.setSid("1001")
        message.setOwnerUserSid("testUser", "DummySID", "testDomain")
        message.setPathname(12,
                            452,
                            6,
                            "/usr/bin/whoami ",
                            (0,0),
                            (0,0),
                            (0,0),
                            (0,0),
                            (0,0),
                            (6,9),
                            (8,0))
        message.setCmdLine("whoami")
        message.setSha256("12ad4434de2701a3c007e5d1a35e116ee113d279cca06540dca5cd22d4a3c691")
        message.setSha1("dabbb6a0debcf4a79b9371125df1428f3e7ca30c")
        self.send_message(ProcessEventChannel, message.serialise())

    def send_message(self, channel, data):
        require_capnpn()
        if self.publisher is None:
            self.start_publisher()
        self.publisher.send_data(0, channel, data)


