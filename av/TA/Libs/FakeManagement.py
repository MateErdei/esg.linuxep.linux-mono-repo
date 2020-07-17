#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.

import zmq
from Libs.PluginCommunicationTools import FakeManagementAgent
from Libs.PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
from Libs.PluginCommunicationTools.common import PathsLocation
from Libs.PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message

import time

class ManagementAgentPluginRequester(object):
    def __init__(self, plugin_name, logger):
        self.name = plugin_name
        self.logger = logger
        self.__m_socket_path = "ipc://{}/plugins/{}.ipc".format(PathsLocation.ipc_dir(), self.name)
        self.__m_socket = try_get_socket(ZMQ_CONTEXT, self.__m_socket_path, zmq.REQ)

    def __del__(self):
        self.__m_socket.close()

    def send_message(self, message):
        to_send = serialise_message(message)
        self.__m_socket.send(to_send)

    def build_message(self, command, app_id, contents):
        return Message(app_id, self.name, command.value, contents)

    def action(self, app_id, correlation, action):
        self.logger.info("Sending {} action [correlation {}] to {} via {}".format(action,
                                                                                  correlation,
                                                                                  self.name,
                                                                                  self.__m_socket_path))
        message = self.build_message(Messages.DO_ACTION, app_id, [action])
        message.correlationId = correlation
        self.send_message(message)

        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)
        if response.acknowledge:
            return Messages.ACK.value
        elif response.error:
            return response.error
        elif response.contents is None:
            return ""
        else:
            return response.contents[0]

    def policy(self, app_id, policy_xml):
        self.logger.info("Sending policy XML to {} via {}, XML:{}".format(self.name, self.__m_socket_path, policy_xml))
        message = self.build_message(Messages.APPLY_POLICY, app_id, [policy_xml])
        self.send_message(message)

        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)
        if response.acknowledge:
            return Messages.ACK.value
        elif response.error:
            return response.error
        elif response.contents is None:
            return ""
        else:
            return response.contents[0]

    def get_status(self, app_id):
        self.logger.info("Request Status for {}".format( self.name))
        request_message = self.build_message(Messages.REQUEST_STATUS, app_id, [])
        self.send_message(request_message)
        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)
        if response.acknowledge or response.error or (response.contents and len(response.contents) < 2):
            self.logger.error("No status in message from plugin: {}, contents: {}".format(response.plugin_name, response.contents))
            return ""
        self.logger.info("Got status back from plugin, plugin name: {}, status: {}".format(self.name, response.contents))

        # This is a list of 2 items: status with XML and status without XML
        status = response.contents[0]
        return status

    def get_telemetry(self):
        self.logger.info("Request Telemetry for {}".format(self.name))
        request_message = self.build_message(Messages.REQUEST_TELEMETRY, "no-app-id", [])
        self.send_message(request_message)
        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)

        if response.acknowledge or response.error or (response.contents and len(response.contents) < 1):
            self.logger.error("No telemetry in message from plugin: {}, contents: {}".format(response.plugin_name, response.contents))
            return ""
        self.logger.info("Got telemetry back from plugin, plugin name: {}, status: {}".format(self.name, response.contents))
        received_telemetry = response.contents[0]
        return received_telemetry

    def send_raw_message(self, to_send):
        self.__m_socket.send_multipart(to_send)
        return self.__m_socket.recv_multipart()


class FakeManagement(object):

    def __init__(self):
        self.logger = FakeManagementAgent.setup_logging("fake_management_agent.log", "Fake Management Agent")
        self.agent = None

    def start_fake_management(self):
        if self.agent:
            raise AssertionError("Agent already initialized")
        self.agent = FakeManagementAgent.Agent(self.logger)
        self.agent.start()

    def stop_fake_management(self):
        if not self.agent:
            self.logger.info("Agent already stopped")
            return
        self.agent.stop()
        self.agent = None

    def send_plugin_policy(self, plugin_name, appid, content):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        plugin.policy(appid, content)

    def send_plugin_action(self, plugin_name, appid, correlation, content):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        plugin.action(appid, correlation, content)

    def get_plugin_status(self, plugin_name, appid):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        return plugin.get_status(appid)

    def wait_for_plugin_status(self, plugin_name, appid, *texts):
        timeout = 15
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        deadline = time.time() + int(timeout)
        status = "FAILED TO FETCH STATUS"

        while time.time() < deadline:
            status = plugin.get_status(appid)
            good = True
            for t in texts:
                if t not in status:
                    good = False
                    break
            if good:
                return

        self.logger.fatal("Plugin status didn't contain expected texts: %s", status)
        raise Exception("Plugin status didn't contain expected texts")


    def get_plugin_telemetry(self, plugin_name):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        return plugin.get_telemetry()

    def __del__(self):
        if self.agent:
            self.stop_fake_management()
