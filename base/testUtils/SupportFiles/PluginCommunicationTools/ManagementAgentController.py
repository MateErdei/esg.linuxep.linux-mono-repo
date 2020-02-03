#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import zmq
from .common.socket_utils import try_get_socket, get_context
from .common.messages import ControlMessagesToAgent


class ManagementAgentController(object):
    def __init__(self, socket_path):
        self.zmq_context = get_context()
        self.controller_socket_path = socket_path
        self.socket = try_get_socket(self.zmq_context, self.controller_socket_path, zmq.REQ)

    def request_status(self, app_id, plugin_name):
        self.socket.send_multipart([ControlMessagesToAgent.REQUEST_STATUS.value, app_id, plugin_name])
        return self.socket.recv_multipart()

    def do_action(self, app_id, plugin_name, action):
        self.socket.send_multipart([ControlMessagesToAgent.DO_ACTION.value, app_id, plugin_name, action])
        return self.socket.recv_multipart()

    def apply_policy(self, app_id, policy_xml):
        self.socket.send_multipart([ControlMessagesToAgent.APPLY_POLICY.value, app_id, policy_xml])
        return self.socket.recv_multipart()

    def request_telemetry(self, plugin_name):
        self.socket.send_multipart([ControlMessagesToAgent.REQUEST_TELEMETRY.value, plugin_name])
        return self.socket.recv_multipart()

    def get_registered_plugins(self):
        self.socket.send_multipart([ControlMessagesToAgent.GET_REGISTERED_PLUGINS.value])
        return self.socket.recv_multipart()

    def set_policy(self, app_id, content):
        self.socket.send_multipart([ControlMessagesToAgent.SET_POLICY.value,  app_id, content])
        return self.socket.recv_multipart()

    def deregister_plugins(self):
        self.socket.send_multipart([ControlMessagesToAgent.DEREGISTER_PLUGINS.value])
        return self.socket.recv_multipart()

    def queue_reply(self, plugin_api_cmd, app_id, payload):
        to_send = [ControlMessagesToAgent.QUEUE_REPLY.value, plugin_api_cmd, app_id]
        for content in payload:
            to_send.append(content)
        self.socket.send_multipart(to_send)
        return self.socket.recv_multipart()

    def send_custom_message(self, app_id, plugin_name, plugin_api_cmd, payload):
        to_send = [ControlMessagesToAgent.CUSTOM_MESSAGE_TO_PLUGIN.value, app_id, plugin_name, plugin_api_cmd]
        for content in payload:
            to_send.append(content)
        self.socket.send_multipart(to_send)
        return self.socket.recv_multipart()

    def stop(self):
        self.socket.send_multipart([ControlMessagesToAgent.STOP_AGENT.value])

    def link_appid_plugin(self, app_id, plugin_name):
        self.socket.send_multipart([ControlMessagesToAgent.LINK_APPID_PLUGIN.value,  app_id, plugin_name])
        return self.socket.recv_multipart()

