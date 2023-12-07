#!/usr/bin/env python
# Copyright 2019-2023 Sophos Limited. All rights reserved.

import zmq
import os

# Just always use python implementation - slower but more compatible
os.environ['PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION'] = 'python'

import grp
import pwd
import time
import datetime
from Libs.PluginCommunicationTools import FakeManagementAgent
from Libs.PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
from Libs.PluginCommunicationTools.common import PathsLocation
from Libs.PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message


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
        filename = "LiveQuery_{}_request_{}.json".format(correlation, self.get_valid_creation_time_and_ttl())
        sophos_install = os.environ['SOPHOS_INSTALL']
        with open(os.path.join(sophos_install,"base/mcs/action/"+filename), "a") as f:
            f.write(action)
        message = self.build_message(Messages.DO_ACTION, app_id, [filename])
        message.correlationId = correlation
        self.send_message(message)

        try:
            raw_response = self.__m_socket.recv()
        except zmq.error.Again as e:
            self.logger.error(str(e))
            return ""
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

    def make_file_readable_by_mcs(self, file_path):
        uid = pwd.getpwnam('sophos-spl-user').pw_uid
        gid = grp.getgrnam('sophos-spl-group')[2]
        os.chown(file_path, uid, gid)

    def get_valid_creation_time_and_ttl(self):
        ttl = int(time.time())+10000
        creation_time = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        return f"{creation_time}_{ttl}"

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

    def send_plugin_actions(self, plugin_name, appid, correlation_prefix, content, count):
        for i in range(count):
            unique_corr_id = f"{correlation_prefix}{i}"
            plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
            plugin.action(appid, unique_corr_id, content)
            time.sleep(0.01)

    def get_plugin_status(self, plugin_name, appid):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        return plugin.get_status(appid)

    def get_plugin_telemetry(self, plugin_name):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        return plugin.get_telemetry()

    def __del__(self):
        if self.agent:
            self.stop_fake_management()
