#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import time
import json
import robot.api.logger
import xml.dom.minidom

import PathManager

SUPPORTFILEPATH = PathManager.get_support_file_path()
PathManager.addPathToSysPath(SUPPORTFILEPATH)

from PluginCommunicationTools import FakePlugin
from PluginCommunicationTools.common.ProtobufSerialisation import Messages


class FakePluginWrapper(object):

    def __init__(self):
        self.__m_pluginName = "FakePlugin"
        self.logger = FakePlugin.setup_logging("fake_plugin.log", self.__m_pluginName)
        self.plugin = None
        self.appId = 'SAV'

    def __del__(self):
        if self.plugin:
            self.stop_plugin()

    def set_fake_plugin_app_id(self, appId):
        self.appId = appId

    def setup_plugin_registry(self):
        config = {
            "policyAppIds": [self.appId],
            "actionAppIds": [self.appId],
            "statusAppIds": [self.appId],
            "pluginName": self.__m_pluginName
        }
        config = json.dumps(config)
        filename = '/opt/sophos-spl/base/pluginRegistry/{}.json'.format(self.__m_pluginName)
        with open(filename, 'w') as f:
            f.write(config)

        robot.api.logger.info("Wrote plugin registry to {}".format(filename))
        robot.api.logger.info(config)

    def remove_fake_plugin_from_registry(self):
        filename = '/opt/sophos-spl/base/pluginRegistry/{}.json'.format(self.__m_pluginName)
        try:
            os.remove(filename)
            robot.api.logger.info("file removed from plugin registry: {}".format(filename))
        except OSError as os_err:
            robot.api.logger.info("Exception: {} while removing plugin registry file: {}".format(str(os_err), filename))

    def start_plugin(self):
        if self.plugin:
            raise AssertionError("Plugin already initialized")
        self.plugin = FakePlugin.FakePlugin(self.logger, self.__m_pluginName, True)
        self.plugin.add_app_id(self.appId)
        self.plugin.start()
        # Wait for plugin thread to start and be ready to send receive messages
        timeout = 10
        while not self.plugin.listening() and timeout > 0:
            time.sleep(0.5)
            timeout -= 0.5
        if not self.plugin.listening():
            raise AssertionError("Plugin not listening after 10 seconds!")

    def stop_plugin(self):
        if not self.plugin:
            raise AssertionError("plugin not initialized")
        self.plugin.stop()
        self.plugin = None

    def send_plugin_request_policy(self):
        message = self.plugin.send_request_policy(self.appId)
        if message.error:
            raise AssertionError("Error message returned: {}".format(message.error))
        elif message.acknowledge:
            return Messages.ACK.value
        elif message.contents is None:
            raise AssertionError("Not an error or acknowledgment with no content!")
        else:
            return message.contents[0]

    def send_plugin_event(self, event_xml):
        try:
            dom = xml.dom.minidom.parseString(event_xml)
            assert dom is not None
        except Exception as e:
            robot.api.logger.warn("Sending invalid XML as an event: {} {}".format(str(e), event_xml))

        self.plugin.send_event(self.appId, [event_xml])

    def send_plugin_status(self, status_content):
        self.plugin.set_status(self.appId, [str(status_content), str(status_content)])
        return self.plugin.send_status_changed(self.appId, status_content, status_content)

    def send_plugin_status_with_refid(self, status_content, policy_file_or_ref_id, res="Same"):
        status_content = self.plugin.apply_refid_to_status(status_content, policy_file_or_ref_id, res)
        return self.send_plugin_status(status_content)

    def get_plugin_action(self):
        action = self.plugin.get_action()
        if action is None:
            raise AssertionError("No action has been sent for this appId: {}".format(self.appId))
        return action

    def get_plugin_policy(self, original_value=""):
        policy = self.plugin.get_policy()
        if policy is None or policy == original_value:
            raise AssertionError("Policy for this appId has not changed: {}".format(self.appId))
        return policy

    def do_everything(self):
        self.start_plugin()

        self.send_plugin_status("content1")
        self.stop_plugin()

    def send_message_to_management_agent_without_protobuf_serialisation(self, message):
        return FakePlugin.send_request_to_management_agent_without_protobuf(message)

    def send_plugin_notification(self, event_xml):
        timestamp = self.create_time_stamp()
        event_xml = event_xml.replace("INSERT_TIME_STAMP", timestamp)

        return self.send_plugin_event(event_xml)

    def create_time_stamp(self):
        """
        :return: Current time in format %Y%m%d %H%M%S"
        """
        return time.strftime("%Y%m%d %H%M%S")
