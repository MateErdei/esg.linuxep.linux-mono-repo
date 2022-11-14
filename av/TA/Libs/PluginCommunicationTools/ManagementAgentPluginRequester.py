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

from .common.socket_utils import try_get_socket, ZMQ_CONTEXT
from .common import PathsLocation
from .common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message


class ManagementAgentPluginRequester(object):
    def __init__(self, plugin_name: str, logger):
        self.name = plugin_name
        self.logger = logger
        self.__m_socket_path = "ipc://{}/plugins/{}.ipc".format(PathsLocation.ipc_dir(), self.name)
        self.__m_socket = try_get_socket(ZMQ_CONTEXT, self.__m_socket_path, zmq.REQ)

    def __del__(self):
        self.__m_socket.close()

    def send_message(self, message: Message):
        to_send = serialise_message(message)
        self.__m_socket.send(to_send)

    def build_message(self, command, app_id, contents) -> Message:
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

        if app_id == "SAV":
            filename = "SAV-2_Policy.xml"
        elif app_id == "ALC":
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
