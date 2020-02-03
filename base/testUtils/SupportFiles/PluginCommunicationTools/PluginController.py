#!/usr/bin/env python

import zmq
from common.socket_utils import try_get_socket
from common.messages import ControlMessagesToPlugin


class PluginController(object):
    def __init__(self, socket_path):
        self.zmq_context = zmq.Context.instance()
        self.controller_socket_path = socket_path
        self.socket = try_get_socket(self.zmq_context, self.controller_socket_path, zmq.REQ)

    def register_plugin(self):
        self.socket.send_multipart([ControlMessagesToPlugin.REGISTER.value])
        return self.socket.recv_multipart()

    def request_policy(self, app_id):
        self.socket.send_multipart([ControlMessagesToPlugin.REQUEST_POLICY.value, app_id])
        return self.socket.recv_multipart()

    def send_event(self, app_id, event_xml):
        self.socket.send_multipart([ControlMessagesToPlugin.SEND_EVENT.value, app_id, event_xml])
        return self.socket.recv_multipart()

    def send_status(self, app_id, status_xml, status_without_xml):
        self.socket.send_multipart([ControlMessagesToPlugin.SEND_STATUS.value, app_id, status_xml, status_without_xml])
        return self.socket.recv_multipart()

    def set_status(self, app_id, status_xml, status_without_xml):
        self.socket.send_multipart([ControlMessagesToPlugin.SET_STATUS.value, app_id, status_xml, status_without_xml])
        return self.socket.recv_multipart()

    def set_telemetry(self, telemetry_xml):
        self.socket.send_multipart([ControlMessagesToPlugin.SET_TELEMETRY.value, telemetry_xml])
        return self.socket.recv_multipart()

    def queue_reply(self, plugin_api_cmd, app_id, payload):
        to_send = [ControlMessagesToPlugin.QUEUE_REPLY.value, plugin_api_cmd, app_id]
        for content in payload:
            to_send.append(content)
        self.socket.send_multipart(to_send)
        return self.socket.recv_multipart()

    def send_custom_message(self, app_id, plugin_name, plugin_api_cmd, payload):
        to_send = [ControlMessagesToPlugin.CUSTOM_MESSAGE_TO_AGENT.value, app_id, plugin_name, plugin_api_cmd]
        for content in payload:
            to_send.append(content)
        self.socket.send_multipart(to_send)
        return self.socket.recv_multipart()

    def stop_plugin(self):
        self.socket.send_multipart([ControlMessagesToPlugin.STOP_PLUGIN.value])
