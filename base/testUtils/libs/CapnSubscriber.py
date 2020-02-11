#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import time
import collections
from FakeMultiSubscriber import FakeMultiSubscriber

import PathManager
SUPPORTFILESPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILESPATH)

try:
    from PluginCommunicationTools.common.CapnpSerialisation import CredentialWrapper, CredentialEventChannel, ProcessWrapper, ProcessEventChannel, AnyDetectorChannel, convert_linux_epoch_to_win32_epoch
    from PluginCommunicationTools.common.SetupLogger import setup_logging
    CAPNPENABLED=True
except Exception as ex:
    CAPNPENABLED=False
    CAPNPNEXCEPTION = ex

def require_capnpn():
    if not CAPNPENABLED:
        raise AssertionError("Capnpn not setup correctly {}".format(CAPNPNEXCEPTION))


LOGGER = setup_logging("CapnSubscriber.log", 'CapnSubscriber')

class CapnSubscriber(object):
    def __init__(self):
        self.subscriber = None
        self.logger = setup_logging("CapnSubscriber", "Capn Subscriber")
        self.message_cache = None

    def clear_subscriber_stop(self):
        require_capnpn()
        LOGGER.info("Capnsubscriber shutdown")
        if self.subscriber:
            self.subscriber.clean_up_multi_subscriber()
            self.subscriber = None

    def __del__(self):
        LOGGER.info("Capnsubscriber shutdown")
        self.clear_subscriber_stop()

    def start_subscriber(self):
        require_capnpn()
        self.subscriber = FakeMultiSubscriber()
        self.subscriber.add_subscriber(AnyDetectorChannel, "DummySubscriber")
        LOGGER.info("Capnsubscriber add subscriber")

    def receive_authentication_fail_with_historic_time(self):
        require_capnpn()
        message = CredentialWrapper()
        byte_string = self.get_message(CredentialEventChannel)
        message.deserialise(byte_string)

        if message.getSessionType() != "network":
            raise AssertionError("Expected sessionType=network got {} instead".format(message.getSessionType()))

        subject_user_sid = message.getSubjectUserSid()
        if (subject_user_sid["username"] != "DummyUser" and
           subject_user_sid["domain"] != "Dummy.Domain" and
           subject_user_sid["SID"] != "DummySID"):
            raise AssertionError("Expected subjectUserSid: username=DummyUser, domain=Dummy.Domian, SID=DummySID subjectUserSid got {} instead".format(message.getSubjectUserSid()))

        target_user_sid = message.getTargetUserSid()
        if (target_user_sid["username"] != "DummyUser" and
                target_user_sid["domain"] != "Dummy.Domain" and
                target_user_sid["SID"] != "DummySID"):
            raise AssertionError("Expected target_user_sid: username=DummyUser, domain=Dummy.Domian, SID=DummySID target_user_sid got {} instead".format(message.getTargetUserSid()))

        if message.getGroupId() != 1000:
            raise AssertionError("Expected groupId=1000 got {} instead".format(message.getGroupId()))

        if message.getGroupName() != "DummyGroup":
            raise AssertionError("Expected userGroupName=DummyGroup got {} instead".format(message.setGroupName()))

        currentTime = convert_linux_epoch_to_win32_epoch(time.time())
        if message.getTimestamp() > currentTime:
            raise AssertionError("Expected timestamp<{} got {} instead".format(currentTime, message.getTimestamp()))

    def recieve_process_event_with_historic_time(self):
        require_capnpn()
        message = ProcessWrapper()
        byte_string = self.get_message(ProcessEventChannel)
        message.deserialise(byte_string)

    def flatten_dictionary(self, d, parent_key='', sep='.'):
        require_capnpn()
        items = []
        for k, v in list(d.items()):
            new_key = parent_key + sep + k if parent_key else k
            if isinstance(v, collections.MutableMapping):
                items.extend(list(self.flatten_dictionary(v, new_key, sep=sep).items()))
            else:
                items.append((new_key, v))
        return dict(items)

    def byte_string_to_flattened_dict(self, byte_string, wrapper):
        require_capnpn()
        wrapper.deserialise(byte_string)
        message_dict = wrapper.get_message_event().to_dict()
        return self.flatten_dictionary(message_dict)

    def subscriber_message_to_dict(self, channel):
        require_capnpn()
        byte_string = self.get_message(channel)
        return self.convert_binary_message_to_dict(byte_string, channel)

    def capn_check_message(self, message, **kwargs):
        require_capnpn()
        for key, value in list(kwargs.items()):
            try:
                current_value = message[key]
                if current_value != value:
                    raise AssertionError("Value expected for key {}: {} instead is {}".format(key, value, current_value))
            except KeyError as ex:
                LOGGER.exception(ex)
                raise AssertionError("Message does not contain " + str(key))

    def convert_binary_message_to_dict(self, message, channel):
        require_capnpn()
        if channel == CredentialEventChannel:
            wrapper = CredentialWrapper()
            return self.byte_string_to_flattened_dict(message, wrapper)
        elif channel == ProcessEventChannel:
            wrapper = ProcessWrapper()
            return self.byte_string_to_flattened_dict(message, wrapper)

    def cache_capn_queue_contents(self, channel):
        require_capnpn()
        self.message_cache = self.subscriber.get_all_messages(0, channel)

    def check_capn_output_for_a_single_message(self, channel, **kwargs):
        require_capnpn()
        if self.message_cache is None:
            self.cache_capn_queue_contents(channel)

        found = False
        for binary_message in self.message_cache:
            message = self.convert_binary_message_to_dict(binary_message, channel)
            for key, value in list(kwargs.items()):
                found = True
                try:
                    current_value = str(message[key])
                    if current_value != value:
                        found = False
                        break
                except KeyError:
                    found = False
                    break
            if found:
                return message

        LOGGER.info("Messages that did not match:")
        for binary_message in self.message_cache:
            message = self.convert_binary_message_to_dict(binary_message, channel)
            LOGGER.info(message)
        raise AssertionError("Could not find the specified message in {} messages.".format(len(self.message_cache)))

    def clear_cache_and_check_capn_output_for_a_single_message(self, channel, **kwargs):
        require_capnpn()
        self.message_cache = None
        self.check_capn_output_for_a_single_message(channel, **kwargs)


    def get_message(self, channel):
        require_capnpn()
        if self.subscriber is None:
            self.start_subscriber()
        LOGGER.info("Capnsubscriber try to get next message")
        try:
            return self.subscriber.get_next_message(0, channel)
        except:
            LOGGER.info("failed to get next message")
            raise

    def subscriber_get_all_messages(self, channel):
        require_capnpn()
        assert (self.subscriber is not None)
        LOGGER.info("Capnsubscriber try to get all messages")
        try:
            messages = self.subscriber.get_all_messages(0, channel)
            dic = [self.convert_binary_message_to_dict(byte_string, channel) for byte_string in messages]
            return dic
        except:
            LOGGER.info("Failed to get all messages")
            raise

    def get_credential_channel(self):
        require_capnpn()
        return CredentialEventChannel

    def get_process_channel(self):
        require_capnpn()
        return ProcessEventChannel