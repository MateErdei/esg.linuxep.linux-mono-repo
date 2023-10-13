#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



from threading import Thread
import zmq

from .common.socket_utils import try_get_socket, ZMQ_CONTEXT
from .common import messages
from .common.ProtobufSerialisation import *
from .common.SetupLogger import setup_logging

class Agent(object):
    def __init__(self, logger):
        # IPC Socket paths
        self.management_agent_socket_path = messages.MANAGEMENT_AGENT_SOCKET_PATH
        # IPC Sockets
        self.management_agent_socket = try_get_socket(ZMQ_CONTEXT, self.management_agent_socket_path, zmq.REP)
        self.__m_running = True
        self.run_thread = None
        self.logger = logger

    def start(self):
        self.__m_running = True
        self.run_thread = Thread(target=self.run)
        self.run_thread.start()

    def stop(self):
        self.__m_running = False
        if self.run_thread is not None:
            self.run_thread.join()
        self.run_thread = None

    def run(self):
        poller = zmq.Poller()
        poller.register(self.management_agent_socket, zmq.POLLIN)

        while self.__m_running:
            try:
                sockets = dict(poller.poll(500))
            except KeyboardInterrupt:
                break

            if self.management_agent_socket in sockets:
                self.handle_plugin_requests()

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

    def handle_request_policy(self, message):
        self.logger.info("Received policy request: {}".format(message))
        message.set_ack()
        self.send_reply_to_plugin(message)

    def send_message_over_agent_socket(self, message):
        to_send = serialise_message(message)
        self.management_agent_socket.send(to_send)

    # Reply to plugin on the management agent socket
    def send_reply_to_plugin(self, message):
        self.logger.info("Sending reply to plugin: {}".format(message))
        self.send_message_over_agent_socket(message)

def main():
    LOGGER = setup_logging("fake_management_agent.log", "Fake Management Agent")
    LOGGER.info("Fake Management Agent started")
    agent = Agent(LOGGER)
    agent.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
