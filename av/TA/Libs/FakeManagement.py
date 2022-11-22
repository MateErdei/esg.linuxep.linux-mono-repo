#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2022 Sophos Plc, Oxford, England.
# All rights reserved.

import grp
import datetime
import os
import pwd
import time
import zmq
try:
    from .PluginCommunicationTools import FakeManagementAgent
    from .PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
    from .PluginCommunicationTools.common import PathsLocation
    from .PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message
    from . import FakeManagementLog
except ImportError:
    from Libs.PluginCommunicationTools import FakeManagementAgent
    from Libs.PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
    from Libs.PluginCommunicationTools.common import PathsLocation
    from Libs.PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message
    from Libs import FakeManagementLog


import traceback


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
        self.logger.info("Sending '{}' action [correlation '{}'] to '{}' via '{}'".format(
                action,
                correlation,
                self.name,
                self.__m_socket_path))
        filename = "ScanNow_Action.xml"
        sophos_install = os.environ['SOPHOS_INSTALL']
        with open(os.path.join(sophos_install, "base/mcs/action/"+filename), "w") as f:
            f.write(action)
        message = self.build_message(Messages.DO_ACTION, app_id, [filename])
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

        if app_id == "sav":
            filename = "SAV-2_Policy.xml"
        elif app_id == "alc":
            filename = "ALC-1_Policy.xml"
        elif app_id == "FLAGS":
            filename = "flags.json"
        else:
            self.logger.fatal("Failed to determine policy type")
            raise Exception("Failed to determine policy type")

        sophos_install = os.environ['SOPHOS_INSTALL']
        with open(os.path.join(sophos_install, "base/mcs/policy/"+filename), "w") as f:
            f.write(policy_xml)
        message = self.build_message(Messages.APPLY_POLICY, app_id, [filename])
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

    def get_status(self, app_id, sleep_period=0.5, max_number_of_retries=10):
        self.logger.info("Request Status for {}".format(self.name))
        request_message = self.build_message(Messages.REQUEST_STATUS, app_id, [])
        self.send_message(request_message)
        raw_response = None
        count = 0
        while count < max_number_of_retries:
            try:
                count += 1
                raw_response = self.__m_socket.recv(flags=0, copy=True)
                break
            except zmq.ZMQError as ex:
                if ex.errno == zmq.EAGAIN:
                    self.logger.error("Got EAGAIN from socket.recv: %d", count)
                    time.sleep(sleep_period)
                    continue
                else:
                    raise
        if raw_response is None:
            self.logger.fatal("Failed to get status from plugin")
            raise Exception("Failed to get status from plugin")
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
        gid = grp.getgrnam('sophos-spl-group').gr_gid
        os.chown(file_path, uid, gid)

    def get_valid_creation_time_and_ttl(self):
        ttl = int(time.time())+10000
        creation_time = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
        return f"{creation_time}_{ttl}"


class FakeManagement(object):
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        self.logger = None
        self.agent = None

    def get_fakemanagement_agent_log_path(self):
        return FakeManagementLog.get_fake_management_log_path()

    def __setup_logger_if_required(self):
        if not os.path.isfile(self.get_fakemanagement_agent_log_path()):
            # Need to restart logging as log file path has disappeared
            self.logger = None

        if self.logger is None:
            self.logger = FakeManagementAgent.setup_logging(FakeManagementLog.get_fake_management_log_filename(), "Fake Management Agent")
        return self.logger

    def start_fake_management(self):
        self.__setup_logger_if_required()
        self.__clean_up_agent_if_not_running()
        if self.agent:
            raise AssertionError("Agent already initialized")
        self.agent = FakeManagementAgent.Agent(self.logger)
        self.agent.start()

    def start_fake_management_if_required(self):
        self.__setup_logger_if_required()
        self.__clean_up_agent_if_not_running()
        if self.agent is None:
            self.logger.info("Starting FakeManagementAgent")
            self.agent = FakeManagementAgent.Agent(self.logger)
            self.agent.start()
        else:
            self.logger.info("FakeManagementAgent already running")

    def stop_fake_management(self):
        if not self.agent:
            if self.logger:
                self.logger.info("Agent already stopped")
            else:
                print("Agent already stopped")
            return
        self.agent.stop()
        self.agent = None
        self.logger = None

    def stop_fake_management_if_running(self):
        if self.agent:
            self.logger.info("Stopping FakeManagementAgent")
            self.agent.stop()
            self.agent = None

    def __clean_up_agent_if_not_running(self):
        if not self.agent:
            return
        if self.agent.is_running():
            return
        self.stop_fake_management_if_running()

    def restart_fake_management(self):
        self.__setup_logger_if_required()
        if self.agent:
            self.agent.stop()
        self.agent = FakeManagementAgent.Agent(self.logger)
        self.agent.start()

    def send_plugin_policy(self, plugin_name, app_id, content):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        plugin.policy(app_id, content)

    def send_plugin_action(self, plugin_name, app_id, correlation, content):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        plugin.action(app_id, correlation, content)

    def get_plugin_status(self, plugin_name, app_id):
        try:
            plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
            return plugin.get_status(app_id)
        except Exception as ex:
            self.logger.error("Failed to get_plugin_status: %s", str(ex))
            trace = traceback.format_exc()
            self.logger.error("Traceback %s", trace)
            raise

    def wait_for_plugin_status(self, plugin_name, app_id, *texts):
        timeout = 15
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        deadline = time.time() + int(timeout)
        status = "FAILED TO FETCH STATUS"

        while time.time() < deadline:
            status = plugin.get_status(app_id)
            good = True
            for t in texts:
                if t not in status:
                    good = False
                    break
            if good:
                return
            time.sleep(0.5)

        self.logger.fatal("Plugin status didn't contain expected texts: %s", status)
        raise Exception("Plugin status didn't contain expected texts")

    def get_plugin_telemetry(self, plugin_name):
        plugin = ManagementAgentPluginRequester(plugin_name, self.logger)
        return plugin.get_telemetry()
