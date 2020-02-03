#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.

from threading import Thread
import json
import zmq

from .common.socket_utils import try_get_socket, ZMQ_CONTEXT
from .common.IPCDir import IPC_DIR
from .common import messages
from .common.messages import *
from .common.ProtobufSerialisation import *


class Plugin(object):
    def __init__(self, plugin_name, logger):
        self.name = plugin_name
        self.logger = logger
        self.__m_socket_path = "ipc://{}/plugins/{}.ipc".format(IPC_DIR, self.name)
        self.__m_socket = try_get_socket(ZMQ_CONTEXT, self.__m_socket_path, zmq.REQ)
        self.__m_events = {}
        self.__m_statuses = {}
        self.__m_telemetries = []

    def send_message(self, message):
        to_send = serialise_message(message)
        self.__m_socket.send(to_send)

    def store_event(self, app_id, event):
        if app_id not in self.__m_events:
            self.__m_events[app_id] = []
        self.__m_events[app_id].append(event)

    def get_events(self, app_id):
        return self.__m_events[app_id]

    def store_status(self, app_id, status):
        if app_id not in self.__m_statuses:
            self.__m_statuses[app_id] = []
        self.__m_statuses[app_id].append(status)

    def get_statuses(self, app_id):
        return self.__m_statuses[app_id]

    def store_telemetry(self, telemetry):
        self.__m_telemetries.append(telemetry)

    def get_telemetries(self):
        return self.__m_telemetries

    def build_message(self, command, app_id, contents):
        return Message(app_id, self.name, command.value, contents)

    def action(self, app_id, action):
        self.logger.info("Sending {} action to {} via {}".format(action, self.name, self.__m_socket_path))
        message = self.build_message(Messages.DO_ACTION, app_id, [action])
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
        self.logger.info("Request Status for " + self.name)
        request_message = self.build_message(Messages.REQUEST_STATUS, app_id, [])
        self.send_message(request_message)
        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)
        if response.acknowledge or response.error or (response.contents and len(response.contents) < 2):
            self.logger.error("No status in message from plugin: {}, contents: {}".format(response.plugin_name, response.contents))
            return ""
        self.logger.info("Got status back from plugin, plugin name: {}, status: {}".format(self.name, str(response.contents)))

        # This is a list of 2 items: status with XML and status without XML
        status = response.contents
        self.store_status(app_id, status)
        return status
    def get_telemetry(self):
        self.logger.info("Request Telemetry for " + self.name)
        request_message = self.build_message(Messages.REQUEST_TELEMETRY, "no-app-id", [])
        self.send_message(request_message)
        raw_response = self.__m_socket.recv()
        response = deserialise_message(raw_response)

        if response.acknowledge or response.error or (response.contents and len(response.contents) < 1):
            self.logger.error("No telemetry in message from plugin: {}, contents: {}".format(response.plugin_name, response.contents))
            return ""
        self.logger.info("Got telemetry back from plugin, plugin name: {}, status: {}".format(self.name, str(response.contents)))
        received_telemetry = response.contents[0].decode("utf-8")
        self.store_telemetry(received_telemetry)
        return received_telemetry

    def send_raw_message(self, to_send):
        self.__m_socket.send_multipart(to_send)
        return self.__m_socket.recv_multipart()


class Agent(object):
    def __init__(self, logger):
        self.app_id_to_plugins = {}
        self.registered_plugins = []

        # IPC Socket paths
        self.management_agent_socket_path = messages.MANAGEMENT_AGENT_SOCKET_PATH
        self.controller_socket_path = messages.MANAGEMENT_AGENT_CONTROLLER_SOCKET_PATH

        # IPC Sockets
        self.management_agent_socket = try_get_socket(ZMQ_CONTEXT, self.management_agent_socket_path, zmq.REP)
        self.controller_socket = try_get_socket(ZMQ_CONTEXT, self.controller_socket_path, zmq.REP)

        self.policies = {}
        self.queued_replies = {}
        self.init_queued_replies()

        self.__m_running = True
        self.run_thread = None
        self.logger = logger

    def init_queued_replies(self):
        for message in Messages:
            self.queued_replies[message.value] = []

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
        poller.register(self.controller_socket, zmq.POLLIN)

        while self.__m_running:
            try:
                sockets = dict(poller.poll(500))
            except KeyboardInterrupt:
                break

            if self.management_agent_socket in sockets:
                self.handle_plugin_requests()
            if self.controller_socket in sockets:
                self.handle_controller_requests()

    def set_policy(self, app_id, policy):
        self.logger.info("Setting policy with APPID: {} to: {}".format(app_id, policy))
        self.policies[app_id] = policy

    def get_policy(self, app_id):
        policy = None
        if app_id in self.policies:
            policy = self.policies[app_id]
        return policy

    def get_plugin(self, plugin_name):
        for plugin in self.registered_plugins:
            if plugin.name == plugin_name:
                return plugin
        return None

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

    def handle_controller_requests(self):
        request_data = self.controller_socket.recv_multipart()

        self.logger.info("Controller request: {}".format(request_data))

        # Create Message object from raw data.
        cmd = ControllerCommand(request_data, ControlMessagesToAgent)

        # Route the messages to handling functions.
        if cmd.action == ControlMessagesToAgent.DO_ACTION:
            self.handle_control_do_action(cmd)
        elif cmd.action == ControlMessagesToAgent.APPLY_POLICY:
            self.handle_control_apply_policy(cmd)
        elif cmd.action == ControlMessagesToAgent.REQUEST_STATUS:
            self.handle_control_request_status(cmd)
        elif cmd.action == ControlMessagesToAgent.REQUEST_TELEMETRY:
            self.handle_control_request_telemetry(cmd)
        elif cmd.action == ControlMessagesToAgent.GET_REGISTERED_PLUGINS:
            self.handle_control_get_registered_plugins()
        elif cmd.action == ControlMessagesToAgent.SET_POLICY:
            self.handle_control_set_policy(cmd)
        elif cmd.action == ControlMessagesToAgent.DEREGISTER_PLUGINS:
            self.handle_control_deregister_plugins()
        elif cmd.action == ControlMessagesToAgent.QUEUE_REPLY:
            self.handle_control_queue_reply(cmd)
        elif cmd.action == ControlMessagesToAgent.CUSTOM_MESSAGE_TO_PLUGIN:
            self.handle_control_send_custom_message(cmd)
        elif cmd.action == ControlMessagesToAgent.STOP_AGENT:
            self.stop()
        elif cmd.action == ControlMessagesToAgent.LINK_APPID_PLUGIN:
            self.handle_control_link_appid_plugin(cmd)
        else:
            self.logger.error("Error: Do not recognise command message type, message: {}".format(cmd))
        return

    # Plugin message handlers and related functions
    def is_plugin_registered(self, plugin_name):
        for plugin in self.registered_plugins:
            if plugin.name == plugin_name:
                return True
        return False

    def handle_register(self, message):
        if message.plugin_name in ["", None]:
            self.logger.error("Plugin tried to register without a plugin name.")
            message.set_error()
            self.send_reply_to_plugin(message)
            return

        if self.is_plugin_registered(message.plugin_name):
            self.logger.info("Already registered plugin registered again: {}".format(message.plugin_name))
            plugin = self.get_plugin(message.plugin_name)
        else:
            plugin = Plugin(message.plugin_name, self.logger)
            self.registered_plugins.append(plugin)

        message.set_ack()
        self.send_reply_to_plugin(message)
        self.logger.info("Registered plugin: {}".format(plugin.name))

    # This handles the plugin send event, i.e. agent received event from plugin.
    def handle_send_event(self, message):
        self.logger.info("Received event: {}".format(message))
        if len(message.contents) < 1:
            error_msg = "No contents set in event message"
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return

        plugin = self.get_plugin(message.plugin_name)
        if plugin is None:
            error_msg = "Could not find a registered plugin for to store the received event against."
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return

        event = message.contents[0]
        plugin.store_event(message.app_id, event)
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

        plugin = self.get_plugin(message.plugin_name)
        if plugin is None:
            error_msg = "Could not find a registered plugin for to store the received status against."
            self.logger.error(error_msg)
            message.set_error_with_payload(error_msg)
            self.send_reply_to_plugin(message)
            return

        plugin.store_status(message.app_id, message.contents)
        message.set_ack()
        self.send_reply_to_plugin(message)

    def handle_request_policy(self, message):
        self.logger.info("Received policy request: {}".format(message))
        policy = self.get_policy(message.app_id)
        if policy is None:
            log_message = "No policy with that APPID: {}".format(message.app_id)
            self.logger.warn(log_message)
            message.set_error_with_payload(log_message)
        else:
            message.contents.append(policy)

        self.send_reply_to_plugin(message)

    # Assumes that contents is always set in the fake message (which will be response with or without a payload)
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
        self.management_agent_socket.send(to_send)

    # Reply to plugin on the management agent socket
    def send_reply_to_plugin(self, message):
        queued_reply = self.get_next_queued_reply_for_command(message.command, message)

        # If we have a queued reply, send it
        if queued_reply is not None:
            self.logger.info("Sending faked reply to plugin: {}".format(queued_reply))
            self.send_message_over_agent_socket(queued_reply)
            return

        # Else just send message as normal
        self.logger.info("Sending reply to plugin: {}".format(message))
        self.send_message_over_agent_socket(message)

# Controller message handlers and related functions
    def send_message_to_controller(self, content):
        if isinstance(content, list):
            self.controller_socket.send_multipart(content)
        else:
            self.controller_socket.send_multipart([content])

    def handle_control_do_action(self, control_message):
        self.logger.info("Received controller do action command: {}".format(control_message))
        app_id = control_message.get_argument(0)
        plugin_name = control_message.get_argument(1)
        action = control_message.get_argument(2)
        try:
            response = self.send_action(plugin_name, app_id, action)
            self.send_message_to_controller(response)
        except Exception as ex:
            self.logger.error(str(ex))
            self.send_message_to_controller(str(ex))
            return

    def send_action(self, plugin_name, app_id, action):
        plugin = self.get_plugin(plugin_name)
        if plugin is None:
            error_msg = "Error: handle_control_do_action: Could not find a plugin with name: {}".format(plugin_name)
            raise RuntimeError(error_msg)

        response = plugin.action(app_id, action)
        return response

    def handle_control_apply_policy(self, control_message):
        self.logger.info("Received controller apply policy command: {}".format(control_message))
        app_id = control_message.get_argument(0)
        policy = control_message.get_argument(1)
        responses = self.send_apply_policy(app_id, policy)
        self.send_message_to_controller(responses)


    def send_apply_policy(self, app_id, policy):

        # Remember the policy that was sent to the plugin and override the stored one for that APPID, this behaviour may change.
        self.set_policy(app_id, policy)

        if not self.registered_plugins:
            raise RuntimeError("No plugin registered")

        plugin_names = self.app_id_to_plugins[app_id]
        if not plugin_names:
            raise RuntimeError("No plugin name for the given appId: " + str(app_id))

        responses = []
        for p in self.registered_plugins:
            if p.name in plugin_names:
                responses.append(p.policy(app_id, policy))
        if not responses:
            raise RuntimeError("No response received")

        return responses


    def handle_control_request_status(self, control_message):
        self.logger.info("Received controller request status command: {}".format(control_message))
        app_id = control_message.get_argument(0)
        pluginname = control_message.get_argument(1)
        try:
            received_status = self.get_status(app_id, pluginname)
            self.send_message_to_controller(received_status)
        except Exception as ex:
            self.logger.error(str(ex))
            self.send_message_to_controller(str(ex))


    def get_status(self, appid, plugin_name):
        plugin = self.get_plugin(plugin_name)
        if plugin is None:
            error_msg = "Error: handle_control_request_status: Could not find a plugin with name: {}".format(plugin_name)
            raise error_msg
        return plugin.get_status(appid)




    def handle_control_request_telemetry(self, control_message):
        self.logger.info("Received controller request telemetry command: {}".format(control_message))
        pluginname = control_message.get_argument(0)
        try:
            received_telemetry = self.get_telemetry(pluginname)
            self.send_message_to_controller(received_telemetry)
        except Exception as ex:
            self.logger.error(str(ex))
            self.send_message_to_controller(str(ex))

    def get_telemetry(self, pluginname):

        plugin = self.get_plugin(pluginname)
        if plugin is None:
            error_msg = "Error: handle_control_request_telemetry: Could not find a plugin with name: {}".format(pluginname)
            raise RuntimeError(error_msg)
        return plugin.get_telemetry()

    def handle_control_get_registered_plugins(self):
        self.logger.info("Received controller get registered plugins command")
        plugins = []
        for plugin in self.registered_plugins:
            plugins.append(plugin.name)
        plugins_json_string = json.dumps(plugins)
        self.send_message_to_controller(plugins_json_string)

    def handle_control_set_policy(self, control_message):
        self.logger.info("Received controller set policy command: {}".format(control_message))
        app_id = control_message.get_argument(0)
        policy = control_message.get_argument(1)
        self.set_policy(app_id, policy)
        self.send_message_to_controller(Messages.ACK.value)

    def handle_control_deregister_plugins(self):
        self.registered_plugins = []
        self.send_message_to_controller("Deregistered all plugins")

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
        payload = control_message.args[2:]
        plugin_name = None
        fake_reply = Message(app_id, plugin_name, plugin_api_command_enum.value, payload)

        self.logger.info("Queue reply: {}".format(fake_reply))
        self.queued_replies[plugin_api_command_enum.value].append(fake_reply)
        self.send_message_to_controller(Messages.ACK.value)

    def handle_control_send_custom_message(self, control_message):

        plugin_name = control_message.get_argument(0)

        # Skip first one as that is the plugin name which have used here to find the correct socket.
        fields_to_send = control_message.args[1:]

        self.logger.info("Received controller send custom message to plugin: {}, contents: {}".format(plugin_name, fields_to_send))
        plugin = self.get_plugin(plugin_name)
        if plugin is None:
            error_msg = "Error: handle_control_send_custom_message: Could not find a plugin with name: {}".format(control_message.get_argument(0))
            self.logger.error(error_msg)
            self.send_message_to_controller(error_msg)
            return

        response = plugin.send_raw_message(fields_to_send)
        self.send_message_to_controller(str(response))

    def link_app_id_to_plugin(self, app_id, plugin_name):
        if app_id not in self.app_id_to_plugins:
            self.app_id_to_plugins[app_id] = []

        if plugin_name not in self.app_id_to_plugins[app_id]:
            self.app_id_to_plugins[app_id].append(plugin_name)

    def handle_control_link_appid_plugin(self, control_message):
        app_id = control_message.get_argument(0)
        plugin_name = control_message.get_argument(1)
        self.link_app_id_to_plugin(app_id, plugin_name)
        self.send_message_to_controller(Messages.ACK.value)


def main():
    LOGGER = setup_logging("fake_management_agent.log", "Fake Management Agent")
    LOGGER.info("Fake Management Agent started")
    agent = Agent(LOGGER)
    agent.run()
    return 0


if __name__ == "__main__":
    sys.exit(main())
