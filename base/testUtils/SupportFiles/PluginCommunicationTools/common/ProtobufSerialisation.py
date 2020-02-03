#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.




import sys
import os
from enum import Enum
from .SetupLogger import setup_logging
from .messages import Message

SYSTEMPRODUCTTESTOUTPUT = os.path.join(os.path.dirname(__file__), "..", "..", "..", "SystemProductTestOutput")
if SYSTEMPRODUCTTESTOUTPUT not in sys.path:
    sys.path.append(SYSTEMPRODUCTTESTOUTPUT)

# Test we actually have protobuf before trying to import the message
import google.protobuf

# The following imports are made from the SystemProductTestOutput folder which is generated
# during the initialisation of the robot tests. If you wish to use this in isolation you will need
# to copy the SystemProductTestOutput folder from a build of base into the root of the System Product Tests
# See libs/SystemProductTestOutputInstall.py for more information.
from PluginAPIMessage_pb2 import PluginAPIMessage

LOGGER = setup_logging("protobuf_processing", "protobuf log")

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
    INVALID_COMMAND = PluginAPIMessage.InvalidCommand
    # both
    ACK = 'ACK'
    ERROR = "Error"


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
    if pluginapimessage.HasField("acknowledge"):
        message.acknowledge = pluginapimessage.acknowledge
    message.contents = []
    for item in pluginapimessage.payload:
      message.contents.append(item.encode("ascii"))
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
    elif message.correlationId:
        pluginapimessage.correlationId = message.correlationId
    else:
        for item in message.contents:
            pluginapimessage.payload.append(item)
    return pluginapimessage.SerializeToString()