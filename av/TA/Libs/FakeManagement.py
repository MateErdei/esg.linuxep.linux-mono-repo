#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import time
import traceback


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
        if app_id == "sav":
            self.logger.info("Upgrading appid 'sav' to 'SAV'")
            app_id = "SAV"
        plugin.policy(app_id, content)
        if self.agent is not None:
            self.agent.set_default_policy(app_id, content)

    def send_plugin_policy_from_file(self, plugin_name, app_id, path):
        content = open(path).read()
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
