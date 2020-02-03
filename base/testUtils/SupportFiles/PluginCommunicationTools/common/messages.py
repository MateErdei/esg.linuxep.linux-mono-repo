#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



from enum import Enum

from .SetupLogger import setup_logging
from . import IPCDir

LOGGER = setup_logging("messages_processing", "messages log")


INSTALL_LOCATION = IPCDir.INSTALL_LOCATION
IPC_DIR = IPCDir.IPC_DIR

MANAGEMENT_AGENT_SOCKET_PATH = "ipc://{}/mcs_agent.ipc".format(IPC_DIR)
MANAGEMENT_AGENT_CONTROLLER_SOCKET_PATH = "ipc://{}/fake_management_controller.ipc".format(IPC_DIR)

class ControlMessagesToAgent(Enum):
    DO_ACTION = b'PluginDoAction'
    APPLY_POLICY = b'PluginApplyPolicy'
    REQUEST_STATUS = b'PluginRequestStatus'
    REQUEST_TELEMETRY = b'PluginRequestTelemetry'
    GET_REGISTERED_PLUGINS = b'AgentGetRegisteredPlugins'
    SET_POLICY = b'AgentSetPolicy'
    DEREGISTER_PLUGINS = b'DeregisterPlugins'
    QUEUE_REPLY = b'QueueReply'
    CUSTOM_MESSAGE_TO_PLUGIN = b'SendCustomMessageToPlugin'
    LINK_APPID_PLUGIN = b'LinkAppIDPlugin'
    STOP_AGENT = b'Stop'


class ControlMessagesToPlugin(Enum):
    REGISTER = b'RegisterPlugin'
    SEND_EVENT = b'SendEventToAgent'
    SEND_STATUS = b'SendStatusToAgent'
    REQUEST_POLICY = b'RequestPolicyFromAgent'
    SET_STATUS = b'SetPluginStatus'
    SET_TELEMETRY = b'SetPluginTelemetry'
    QUEUE_REPLY = b'QueueReply'
    CUSTOM_MESSAGE_TO_AGENT = b'SendCustomMessageToAgent'
    STOP_PLUGIN = b'Stop'

class Message(object):
    def __init__(self, app_id=None, plugin_name=None, command=None, contents=None, acknowledge=False, error=None):
        self.app_id = app_id
        self.plugin_name = plugin_name
        self.command = command
        self.acknowledge = acknowledge
        self.error = error
        self.contents = contents
        self.correlationId = ""

    def set_ack(self):
        self.acknowledge = True

    def set_error(self):
        self.error = "Error"

    def set_error_with_payload(self, error_message):
        self.error = error_message

    def __str__(self):
        if self.correlationId:
            return "APPID={}, Command={}, Contents={}, Acknowledge={}, Error={}, CorrelationId={}".format(
                self.app_id, self.command, self.contents, self.acknowledge, self.error, self.correlationId)
        else:
            return "APPID={}, Command={}, Contents={}, Acknowledge={}, Error={}".format(self.app_id, self.command, self.contents, self.acknowledge, self.error)


# Commands from the controller script
class ControllerCommand(object):
    def __init__(self, message, enums):
        if len(message) < 1:
            LOGGER.error("Error: Control command sent to Fake Management Agent is malformed: {}".format(message))
            return

        # We must always have an action
        raw_action = message[0]
        self.enums = enums
        self.action = self.get_enum_from_raw_action(raw_action)
        if self.action is None:
            LOGGER.error("Unrecognised action string: {}".format(raw_action))

        self.args = message[1:]

    def get_argument(self, index):
        if len(self.args) <= index:
            return None
        else:
            return self.args[index]

    def get_enum_from_raw_action(self, raw_action):
        for enum in self.enums:
            if enum.value == raw_action:
                return enum
        return None

    def __str__(self):
        return "Action={}, Contents={}".format(self.action, str(self.args))




