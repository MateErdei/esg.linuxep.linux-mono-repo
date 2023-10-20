#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2018-2023 Sophos Limited. All rights reserved.
# All rights reserved.

import os
import sys
import time
import traceback


def append_path(p):
    if p not in sys.path:
        sys.path.append(p)


THIS_FILE_PATH = os.path.realpath(__file__)
LIBS_DIR = os.path.dirname(THIS_FILE_PATH)
append_path(LIBS_DIR)
TA_DIR = os.path.dirname(LIBS_DIR)
append_path(TA_DIR)

try:
    from .PluginCommunicationTools import FakeManagementAgent
    from .PluginCommunicationTools import ManagementAgentPluginRequester
    from .PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
    from .PluginCommunicationTools.common import PathsLocation
    from .PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message
    from . import FakeManagementLog
except ImportError:
    try:
        from Libs.PluginCommunicationTools import FakeManagementAgent
        from Libs.PluginCommunicationTools import ManagementAgentPluginRequester
        from Libs.PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
        from Libs.PluginCommunicationTools.common import PathsLocation
        from Libs.PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message
        from Libs import FakeManagementLog
    except ImportError:
        from PluginCommunicationTools import FakeManagementAgent
        from PluginCommunicationTools import ManagementAgentPluginRequester
        from PluginCommunicationTools.common.socket_utils import try_get_socket, ZMQ_CONTEXT
        from PluginCommunicationTools.common import PathsLocation
        from PluginCommunicationTools.common.ProtobufSerialisation import Message, Messages, deserialise_message, serialise_message
        import FakeManagementLog

PLUGIN_NAME = "av"


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

    def __create_agent(self):
        self.agent = FakeManagementAgent.Agent(self.logger, PLUGIN_NAME)
        # set default policies

        return self.agent

    def start_fake_management(self):
        self.__setup_logger_if_required()
        self.__clean_up_agent_if_not_running()
        if self.agent:
            raise AssertionError("Agent already initialized")
        self.__create_agent()
        self.agent.start()

    def start_fake_management_if_required(self):
        self.__setup_logger_if_required()
        self.__clean_up_agent_if_not_running()
        if self.agent is None:
            self.logger.info("Starting FakeManagementAgent")
            self.agent = self.__create_agent()
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
        self.agent = self.__create_agent()
        self.agent.start()

    def __get_requester(self, plugin_name):
        if self.agent is not None:
            return self.agent.get_requester()
        return ManagementAgentPluginRequester.ManagementAgentPluginRequester(plugin_name, self.logger)

    def send_plugin_policy(self, plugin_name, app_id, content):
        plugin = self.__get_requester(plugin_name)
        if app_id.islower():
            self.logger.error("appid:{} is lower case".format(app_id))
            app_id = app_id.upper()

        if self.agent is not None:
            self.agent.set_default_policy(app_id, content)
        else:
            self.logger.error("send_plugin_policy without agent active")

        plugin.policy(app_id, content)

    def set_default_policy(self, app_id, content):
        if self.agent is None:
            raise AssertionError("Trying to set default policy when agent not running")
        self.agent.set_default_policy(app_id, content)

    def set_default_policy_from_file(self, app_id, path):
        content = open(path).read()
        return self.set_default_policy(app_id, content)

    def send_plugin_policy_from_file(self, plugin_name, app_id, path):
        content = open(path).read()
        if content == "":
            self.logger.error("Attempting to send empty policy for "+app_id+" from "+path)
            raise AssertionError("Attempting to send empty policy for "+app_id+" from "+path)
        self.logger.info(app_id+" policy = "+content)
        return self.send_plugin_policy(plugin_name, app_id, content)

    def send_av_policy(self, app_id, content):
        return self.send_plugin_policy(PLUGIN_NAME, app_id, content)

    def send_av_policy_from_file(self, app_id, path):
        return self.send_plugin_policy_from_file(PLUGIN_NAME, app_id, path)

    def send_plugin_action(self, plugin_name, app_id, correlation, content):
        plugin = self.__get_requester(plugin_name)
        plugin.action(app_id, correlation, content)

    def get_plugin_status(self, plugin_name, app_id):
        try:
            plugin = self.__get_requester(plugin_name)
            return plugin.get_status(app_id)
        except Exception as ex:
            self.logger.error("Failed to get_plugin_status: %s", str(ex))
            trace = traceback.format_exc()
            self.logger.error("Traceback %s", trace)
            raise

    def wait_for_plugin_status(self, plugin_name, app_id, *texts):
        timeout = 15
        plugin = self.__get_requester(plugin_name)
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
        plugin = self.__get_requester(plugin_name)
        return plugin.get_telemetry()
