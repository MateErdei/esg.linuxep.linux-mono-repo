#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.
from enum import Enum
from .SetupLogger import get_logger
from .messages import Message

from .PluginAPIMessage_pb2 import PluginAPIMessage

LOGGER = get_logger("protobuf_processing")

class Messages(Enum):
    # from management agent
    DO_ACTION = PluginAPIMessage.DoAction
    APPLY_POLICY = PluginAPIMessage.ApplyPolicy
    REQUEST_STATUS = PluginAPIMessage.RequestStatus
    REQUEST_TELEMETRY = PluginAPIMessage.Telemetry
    # to management agent
    REGISTRATION = PluginAPIMessage.Registration
    SEND_EVENT = PluginAPIMessage.SendEvent
    SEND_STATUS = PluginAPIMessage.SendStatus
    REQUEST_POLICY = PluginAPIMessage.RequestCurrentPolicy
    SEND_THREAT_HEALTH = PluginAPIMessage.SendThreatHealth
    INVALID_COMMAND = PluginAPIMessage.InvalidCommand
    # both
    ACK = b'ACK'
    ERROR = b"Error"


def deserialise_message(serialised_message):
    pluginapimessage = PluginAPIMessage()
    pluginapimessage.ParseFromString(serialised_message)
    message = Message()
    if pluginapimessage.HasField("applicationId"):
        message.app_id = pluginapimessage.applicationId

    if pluginapimessage.HasField("pluginName"):
        message.plugin_name = pluginapimessage.pluginName

    if pluginapimessage.HasField("command"):
        message.command = pluginapimessage.command

    if pluginapimessage.HasField("error"):
        message.error = pluginapimessage.error
    if pluginapimessage.HasField("correlationId"):
        message.correlationId = pluginapimessage.correlationId
    elif pluginapimessage.HasField("acknowledge"):
        message.acknowledge = pluginapimessage.acknowledge
    else:
        message.contents = []
        for item in pluginapimessage.payload:
            message.contents.append(item)
    return message

def serialise_message(message):
    pluginapimessage = PluginAPIMessage()
    pluginapimessage.pluginName = message.plugin_name
    pluginapimessage.applicationId = message.app_id
    pluginapimessage.command = message.command
    if message.error:
        pluginapimessage.error = message.error
    elif message.acknowledge:
        pluginapimessage.acknowledge = message.acknowledge
    else:
        for item in message.contents:
            pluginapimessage.payload.append(item)
    if message.correlationId != "":
        pluginapimessage.correlationId = message.correlationId
    return pluginapimessage.SerializeToString()