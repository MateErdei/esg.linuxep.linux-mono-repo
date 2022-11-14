#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum


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


