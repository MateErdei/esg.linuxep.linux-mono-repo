#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import zmq
from threading import Thread
import re
import xml.dom.minidom

from .common.messages import *
from .common.socket_utils import try_get_socket, ZMQ_CONTEXT
from .common.utils import to_byte_string
from .common.ProtobufSerialisation import *

class FakePlugin(object):
    def __init__(self, logger, name, register_on_startup):
        self.logger = logger
        self.name = name
        self.app_ids = []
        self.register_on_startup = register_on_startup

        # IPC Socket paths
        self.management_agent_socket_path = MANAGEMENT_AGENT_SOCKET_PATH
        self.plugin_socket_path = "ipc://{}/plugins/{}.ipc".format(IPC_DIR, self.name)
        self.controller_socket_path = "ipc://{}/fake_plugin_controller_{}.ipc".format(IPC_DIR, self.name)

        # IPC Sockets
        self.management_agent_socket = try_get_socket(ZMQ_CONTEXT, self.management_agent_socket_path, zmq.REQ)
        self.controller_socket = try_get_socket(ZMQ_CONTEXT, self.controller_socket_path, zmq.REP)
        self.plugin_socket = try_get_socket(ZMQ_CONTEXT, self.plugin_socket_path, zmq.REP)

        self.action = None
        self.policy = None
        self.statuses = {}
        self.telemetry = None
        self.run_thread = None
        self.receiving_messages  = False

        self.__m_running = True

        # This is of the form queued_replies["actionX"] = [messageX1, messageX2], queued_replies["actionY"] = [messageY1, messageY2]
        self.queued_replies = {}
        self.init_queued_replies()

    def init_queued_replies(self):
        for message in Messages:
            self.queued_replies[message.value] = []

    def add_app_id(self, app_id):
        self.app_ids.append(app_id)

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
        poller.register(self.controller_socket, zmq.POLLIN)
        poller.register(self.plugin_socket, zmq.POLLIN)

        if self.register_on_startup:
            self.register_plugin()

        while self.__m_running:
            try:
                sockets = dict(poller.poll(500))
            except KeyboardInterrupt:
                break

            if self.plugin_socket in sockets:
                self.handle_management_agent_requests()
            if self.controller_socket in sockets:
                self.handle_controller_requests()
            self.receiving_messages = True

    def listening(self):
        return self.receiving_messages

    def set_action(self, action):
        self.action = action

    def get_action(self):
        return self.action

    def set_policy(self, policy):
        self.policy = policy

    def get_policy(self):
        return self.policy

    def set_status(self, app_id, status):
        # type: (object, object) -> None
        self.statuses[app_id] = status

    def get_status(self, app_id):
        if app_id in self.statuses:
            return self.statuses[app_id]
        return None

    def get_telemetry(self):
        return self.telemetry

    def set_telemetry(self, telemetry):
        self.telemetry = telemetry

    def build_message(self, command, app_id, contents):
        return Message(app_id, self.name, command, contents)

    def register_plugin(self):
        register_message = self.build_message(Messages.REGISTRATION.value, "", [])
        response = self.send_message_over_agent_socket(register_message)
        if response.error is not None or not response.acknowledge:
            self.logger.error("Error : During registration expected an acknowledgement or error from management agent but received nothing")
        if response.acknowledge:
            self.logger.info("Registration Successful")
        return response

    def handle_management_agent_requests(self):
        request_data = self.plugin_socket.recv()
        self.logger.info("Plugin request: {}".format(request_data))

        # Create Message object from raw data.
        message = deserialise_message(request_data)

        # Route the messages to handling functions.
        if message.command == Messages.DO_ACTION.value:
            self.handle_do_action(message)
        elif message.command == Messages.APPLY_POLICY.value:
            self.handle_apply_policy(message)
        elif message.command == Messages.REQUEST_STATUS.value:
            self.handle_request_status(message)
        elif message.command == Messages.REQUEST_TELEMETRY.value:
            self.handle_request_telemetry(message)
        else:
            self.logger.error("Error: Do not recognise message type, message: {}".format(message))
        return

    def handle_controller_requests(self):
        request_data = self.controller_socket.recv_multipart()

        self.logger.info("Controller request: {}".format(request_data))

        # Create Message object from raw data.
        cmd = ControllerCommand(request_data, ControlMessagesToPlugin)

        # Route the messages to handling functions.
        if cmd.action == ControlMessagesToPlugin.REGISTER:
            self.handle_control_register(cmd)
        elif cmd.action == ControlMessagesToPlugin.SEND_EVENT:
            self.handle_control_send_event(cmd)
        elif cmd.action == ControlMessagesToPlugin.SEND_STATUS:
            self.handle_control_send_status(cmd)
        elif cmd.action == ControlMessagesToPlugin.REQUEST_POLICY:
            self.handle_control_request_policy(cmd)
        elif cmd.action == ControlMessagesToPlugin.SET_STATUS:
            self.handle_control_set_status(cmd)
        elif cmd.action == ControlMessagesToPlugin.SET_TELEMETRY:
            self.handle_control_set_telemetry(cmd)
        elif cmd.action == ControlMessagesToPlugin.QUEUE_REPLY:
            self.handle_control_queue_reply(cmd)
        elif cmd.action == ControlMessagesToPlugin.CUSTOM_MESSAGE_TO_AGENT:
            self.handle_control_send_custom_message(cmd)
        elif cmd.action == ControlMessagesToPlugin.STOP_PLUGIN:
            self.stop()
        else:
            self.logger.error("Error: Do not recognise command message type, message: {}".format(cmd))
        return

    # Plugin message handlers for message received from management agent
    def handle_do_action(self, message):
        self.logger.info("Received do action: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No filename for action"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_management_agent(message)
            return
        filepath = os.path.join("/opt/sophos-spl/base/mcs/action", message.contents[0].decode())
        with open(filepath, 'r') as f:
            content = f.read()
        if len(content) < 1:
            error_msg = "No contents set in do action message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_management_agent(message)
            return
        self.set_action(content)
        message.set_ack()
        message.contents = [self.get_action()]
        self.send_reply_to_management_agent(message)

    def handle_apply_policy(self, message):
        self.logger.info("Received apply policy: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No filename for policy"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_management_agent(message)
            return
        filepath = os.path.join("/opt/sophos-spl/base/mcs/policy", message.contents[0].decode())
        with open(filepath, 'r') as f:
            content = f.read()
        if len(content) < 1:
            error_msg = "No contents set in apply policy message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_management_agent(message)
            return
        self.set_policy(content)
        message.set_ack()
        message.contents = [self.get_policy()]
        self.send_reply_to_management_agent(message)

    def handle_request_status(self, message):
        self.logger.info("Received request status request: {}".format(message))
        status = self.get_status(message.app_id)
        if status is None:
            log_message = "No status is set on plugin"
            self.logger.warn(log_message)
            message.set_error_with_payload(log_message)
        else:
            message.contents = status

        self.send_reply_to_management_agent(message)

    def handle_request_telemetry(self, message):
        self.logger.info("Received request telemetry request: {}".format(message))
        telemetry = self.get_telemetry()
        if telemetry is None:
            log_message = "No telemetry is set on plugin"
            self.logger.warn(log_message)
            message.set_error_with_payload(log_message)
        else:
            message.contents.append(telemetry)

        self.send_reply_to_management_agent(message)

    # Assumes that contents is always set in the fake message
    def get_next_queued_reply_for_command(self, command, message_to_merge):
        if len(self.queued_replies[command]) == 0:
            return None
        fake_reply = self.queued_replies[command].pop(0)

        # Merge fake reply and message
        if fake_reply.plugin_name in [None, ""]:
            fake_reply.plugin_name = message_to_merge.plugin_name

        if fake_reply.app_id in [None, ""]:
                fake_reply.app_id = message_to_merge.app_id

        return fake_reply

    def send_message_over_agent_socket(self, message):
        to_send = serialise_message(message)
        # Attempt to send message. If it times out recreate the socket
        self.management_agent_socket.send(to_send)
        try:
            raw_response = self.management_agent_socket.recv()
        except zmq.Again as e:
            self.management_agent_socket.close()
            self.management_agent_socket = try_get_socket(ZMQ_CONTEXT, self.management_agent_socket_path, zmq.REQ)
            message.contents = ["Error", "ZMQ didn't connect: {}".format(e)]
            return message

        response = deserialise_message(raw_response)
        if response.error is None and not response.acknowledge and response.contents is None:
            self.logger.error("Error : Expected a response from management agent but received nothing")
        return response

    def send_message_over_plugin_socket(self, message):
        to_send = serialise_message(message)
        self.plugin_socket.send(to_send)

    # Reply to the management agent socket
    def send_reply_to_management_agent(self, message):
        queued_reply = self.get_next_queued_reply_for_command(message.command, message)

        # If we have a queued reply, send it
        if queued_reply is not None:
            self.logger.info("Sending faked reply to plugin: {}".format(queued_reply))
            self.send_message_over_plugin_socket(queued_reply)
            return

        # Else just send message as normal
        self.logger.info("Sending reply to management agent over plugin socket: {}".format(message))
        self.send_message_over_plugin_socket(message)

    # Controller message handlers and related functions
    def send_message_to_controller(self, content):
        self.controller_socket.send_multipart([content])

    def handle_control_register(self, control_message):
        self.logger.info("Received controller register command: {}".format(control_message))
        response = self.register_plugin()
        if response.acknowledge:
            self.send_message_to_controller(Messages.ACK.value)
        else:
            self.send_message_to_controller(Messages.Error.value)

    def send_event(self, app_id, event_xml):
        event_message = self.build_message(Messages.SEND_EVENT.value, app_id, event_xml)
        response = self.send_message_over_agent_socket(event_message)
        return response

    def handle_control_send_event(self, control_message):
        self.logger.info("Received controller send event command: {}".format(control_message))
        response = self.send_event(control_message.args[0], control_message.args[1:])
        if response.acknowledge:
            self.send_message_to_controller(Messages.ACK.value)
        else:
            self.send_message_to_controller(Messages.Error.value)

    def send_status_changed(self, app_id, status_no_xml, status_xml):
        status_message = self.build_message(Messages.SEND_STATUS.value, app_id, [status_no_xml, status_xml])
        response = self.send_message_over_agent_socket(status_message)
        return response

    def handle_control_send_status(self, control_message):
        self.logger.info("Received controller send status command: {}".format(control_message))
        response = self.send_status_changed( control_message.args[0], control_message.args[1], control_message.args[2])
        if response.acknowledge:
            self.send_message_to_controller(Messages.ACK.value)
        else:
            self.send_message_to_controller(Messages.Error.value)

    def send_request_policy(self, app_id):
        request_policy_message = self.build_message(Messages.REQUEST_POLICY.value, app_id, [])
        response = self.send_message_over_agent_socket(request_policy_message)
        return response

    def handle_control_request_policy(self, control_message):
        self.logger.info("Received controller request policy command: {}".format(control_message))
        response = self.send_request_policy(control_message.args[0])
        if response.contents is None or len(response.contents) != 1:
            self.logger.error("Error: Expected just a policy but received {}".format(str(response.contents)))
        else:
            self.set_policy(response.contents[0])
        self.send_message_to_controller(response.contents[0])

    def handle_control_set_status(self, control_message):
        self.logger.info("Received controller set status command: {}".format(control_message))

        app_id = control_message.args[0]
        status_with_xml = control_message.args[1]
        status_without_xml = control_message.args[2]

        self.set_status(app_id, [status_with_xml, status_without_xml])
        self.send_message_to_controller(Messages.ACK.value)

    def handle_control_set_telemetry(self, control_message):
        self.logger.info("Received controller set telemetry command: {}".format(control_message))
        self.set_telemetry(control_message.args[0])
        self.send_message_to_controller(Messages.ACK.value)

    def handle_control_queue_reply(self, control_message):
        plugin_api_command_enum = None
        for enum in Messages:
            if enum.value == int(control_message.get_argument(0)):
                plugin_api_command_enum = enum
                break

        if plugin_api_command_enum is None:
            error_msg = "Tried to queue a reply with a bad Plugin API Action keyword: {}".format(control_message.get_argument(0))
            self.logger.error(error_msg)
            self.send_message_to_controller(error_msg)
            return

        app_id = control_message.get_argument(1)
        contents = control_message.args[2:]
        fake_reply = Message(app_id, self.name, plugin_api_command_enum.value, contents)

        self.logger.info("Queue reply: {}".format(fake_reply))
        self.queued_replies[plugin_api_command_enum.value].append(fake_reply)
        self.send_message_to_controller(Messages.ACK.value)

    def handle_control_send_custom_message(self, control_message):

        plugin_name = control_message.get_argument(0)

        # Skip first one as that is the plugin name which have used here to find the correct socket.
        fields_to_send = control_message.args[1:]

        self.logger.info("Received controller send custom message to management agent: {}, contents: {}".format(plugin_name, fields_to_send))
        response = self.send_message_over_agent_socket(fields_to_send)
        self.send_message_to_controller(str(response))

    def getRevID(self, policy_file_or_ref_id):
        if isinstance(policy_file_or_ref_id,str):
            if len(policy_file_or_ref_id) == 0: ## NoRef
                return ""
            if policy_file_or_ref_id == '""': ## NoRef
                return ""
            if re.match(r"^[a-z0-9]*$",policy_file_or_ref_id):
                return policy_file_or_ref_id

            if "/" in policy_file_or_ref_id:
                path = policy_file_or_ref_id
            else:
                policydir = os.path.join(INSTALL_LOCATION,"base","mcs","policy")
                path = os.path.join(policydir,policy_file_or_ref_id)
            f = open(path)
        else:
            ## Assume a file, or readable
            f = policy_file_or_ref_id

        dom = xml.dom.minidom.parse(f)

        ## find Comp node
        node = dom.getElementsByTagName("csc:Comp")[0]
        return node.getAttribute("RevID")


    def updateCompResNode(self, node, policy_file_or_rev_id, res):

        revId = self.getRevID(policy_file_or_rev_id)

        node.setAttribute("RevID", revId)
        node.setAttribute("Res",res)

    def apply_refid_to_status(self, status_content, policy_file_or_rev_id, res):
        """
        :param status_content: template contnet
        :param policy_file_or_rev_id: file, or rev ID
        :param res: res attribute, Same or Diff
        :return: formatted status content
        """
        dom = xml.dom.minidom.parseString(status_content)
        node = dom.getElementsByTagName("csc:CompRes")[0]
        self.updateCompResNode(node, policy_file_or_rev_id, res)

        status_content = dom.toxml("UTF-8")

        return status_content

def send_request_to_management_agent_without_protobuf(to_send):
    management_agent_socket_path = MANAGEMENT_AGENT_SOCKET_PATH
    management_agent_socket = try_get_socket(ZMQ_CONTEXT, management_agent_socket_path, zmq.REQ)
    management_agent_socket.send(to_byte_string(to_send))
    return management_agent_socket.recv()


def main(argv):
    register_on_startup = False
    plugin_name = "PluginExample"
    app_id = "ALC"
    if len(argv) > 1:
        plugin_name = argv[1]
    if len(argv) > 2:
        app_id = argv[2]
    if len(argv) > 3:
        register_on_startup = str(argv[3]) in ["true", "True", "--register-on-startup"]
    LOGGER = setup_logging(plugin_name, plugin_name)
    plugin = FakePlugin(LOGGER, plugin_name, register_on_startup)
    plugin.add_app_id(app_id)
    LOGGER.info("Fake Plugin started with name: {}, app id: {}, register-on-startup: {}".format(plugin_name, app_id, register_on_startup))
    plugin.run()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

