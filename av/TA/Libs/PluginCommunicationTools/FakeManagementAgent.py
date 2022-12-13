#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
from threading import Thread
import sys
import traceback
import zmq

from .common.socket_utils import try_get_socket, ZMQ_CONTEXT
from .common.ProtobufSerialisation import *
from .common.SetupLogger import setup_logging, get_log_dir
from .common.PathsLocation import management_agent_socket_path

from . import ManagementAgentPluginRequester

class Agent(object):
    def __init__(self, logger, plugin_name):
        # IPC Socket paths
        self.management_agent_socket_path = management_agent_socket_path()
        # IPC Sockets
        self.management_agent_socket = try_get_socket(ZMQ_CONTEXT, self.management_agent_socket_path, zmq.REP)
        self.__m_running = True
        self.run_thread = None
        self.logger = logger
        self.m_policies = {}
        self.__m_plugin_name = plugin_name

    def set_default_policy(self, app_id, content):
        """
        Save a policy as the default to give if the policy is requested
        :param app_id:
        :param content:
        :return:
        """
        self.m_policies[app_id] = content

    def set_default_policy_from_file(self, app_id, path):
        content = open(path).read()
        return self.set_default_policy(app_id, content)

    def socket_path(self):
        return self.management_agent_socket_path

    def is_running(self):
        return self.__m_running

    def start(self):
        self.logger.info("Starting FakeManagementAgent")
        self.__m_running = True
        self.run_thread = Thread(target=self.run)
        self.run_thread.start()

    def stop(self):
        self.logger.info("Stopping FakeManagementAgent")
        self.__m_running = False
        if self.run_thread is not None:
            self.run_thread.join()
        self.run_thread = None

    def run(self):
        poller = zmq.Poller()
        poller.register(self.management_agent_socket, zmq.POLLIN)

        # ZMQ socket doesn't appear immediately
        socket_has_appeared = False

        try:
            while self.__m_running:
                try:
                    sockets = dict(poller.poll(500))
                except KeyboardInterrupt:
                    self.logger.debug("Keyboard Interrupt")
                    break

                if self.management_agent_socket in sockets:
                    self.handle_plugin_requests()

                if os.path.exists(self.socket_path()):
                    self.logger.info("Socket created")
                    socket_has_appeared = True
                elif socket_has_appeared:
                    # Only produce an error after the socket has appeared
                    self.logger.error("Socket path has disappeared!")
                    break

            self.logger.info("Clean exit from FakeManagementAgent")
        except Exception:
            trace = traceback.format_exc()
            self.logger.error("FakeManagementAgent failed with: " + trace)
        finally:
            self.__m_running = False
            self.logger.info("Finishing FakeManagementAgent")

    def handle_plugin_requests(self):
        request_data = self.management_agent_socket.recv()
        self.logger.info("Plugin request: {}".format(request_data))

        # Create Message object from raw data.
        message = deserialise_message(request_data)

        # Route the messages to handling functions.
        if message.command == Messages.REGISTRATION.value:
            self.handle_register(message)
        elif message.command == Messages.SEND_EVENT.value:
            self.handle_send_event(message)
        elif message.command == Messages.REQUEST_POLICY.value:
            self.handle_request_policy(message)
        elif message.command == Messages.SEND_STATUS.value:
            self.handle_send_status(message)
        elif message.command == Messages.SEND_THREAT_HEALTH.value:
            self.handle_send_threat_health(message)
        else:
            self.logger.error("Error: Do not recognise message type, message: {}".format(message))
        return

    def handle_register(self, message):
        if message.plugin_name in ["", None]:
            self.logger.error("Plugin tried to register without a plugin name.")
            message.set_error()
            self.send_reply_to_plugin(message)
            return
        message.set_ack()
        self.send_reply_to_plugin(message)
        self.logger.info("Registered plugin: {}".format(message.plugin_name))

    # This handles the plugin send event, i.e. agent received event from plugin.
    def handle_send_event(self, message):
        self.logger.info("Received event: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No contents set in event message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return

        message.set_ack()
        self.send_reply_to_plugin(message)

    def handle_send_status(self, message):
        self.logger.info("Received status: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No contents set in status message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return
        message.set_ack()
        self.send_reply_to_plugin(message)

    def __get_requester(self):
        return ManagementAgentPluginRequester.ManagementAgentPluginRequester(self.__m_plugin_name, self.logger)

    def handle_request_policy(self, message):
        self.logger.info("Received policy request: {}".format(message))
        message.set_ack()
        self.send_reply_to_plugin(message)
        policy = self.m_policies.get(message.app_id, None)
        if policy is not None:
            self.logger.info("Sending back policy: {}".format(policy))
            plugin = self.__get_requester()
            try:
                plugin.policy(message.app_id, policy)
            except Exception as ex:
                self.logger.warn("Failed to send policy: %s" % str(ex))

    def handle_send_threat_health(self, message):
        self.logger.info("Received threat health: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No contents set in threat health message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return
        message.set_ack()
        self.send_reply_to_plugin(message)

    def send_message_over_agent_socket(self, message):
        to_send = serialise_message(message)
        self.management_agent_socket.send(to_send)

    # Reply to plugin on the management agent socket
    def send_reply_to_plugin(self, message):
        self.logger.info("Sending reply to plugin: {}".format(message))
        self.send_message_over_agent_socket(message)

    def get_requester(self):
        return ManagementAgentPluginRequester.ManagementAgentPluginRequester(self.__m_plugin_name, self.logger)

    def send_policy(self, app_id):
        """
        Send policies
        :param app_id:
        :return:
        """
        content = self.m_policies.get(app_id, None)
        if content is None:
            return

        plugin = self.get_requester()
        plugin.policy(app_id, content)


def main():
    LOGGER = setup_logging("fake_management_agent.log", "Fake Management Agent")
    LOGGER.info("Fake Management Agent started")
    agent = Agent(LOGGER)
    agent.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
