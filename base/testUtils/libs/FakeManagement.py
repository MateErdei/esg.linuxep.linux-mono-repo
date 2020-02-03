#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2020 Sophos Plc, Oxford, England.
# All rights reserved.



import os
import time

import PathManager
SUPPORTFILESPATH = os.path.join(os.path.dirname(__file__), "..", "SupportFiles")
PathManager.addPathToSysPath(SUPPORTFILESPATH)


from PluginCommunicationTools import FakeManagementAgent


class FakeManagement(object):

    def __init__(self):
        self.logger = FakeManagementAgent.setup_logging("fake_management_agent.log", "Fake Management Agent")
        self.agent = None
        pass

    def _get_file_content(self, filename_or_path):
        if os.path.exists(filename_or_path):
            filepath = filename_or_path
        else:
            filepath = os.path.join(SUPPORTFILESPATH, "CentralXml", filename_or_path + ".xml")
            if not os.path.exists(filepath):
                raise AssertionError("Policy file not found: " + filepath)
        return open(filepath, 'r').read()


    def start_fake_management(self):
        if self.agent:
            raise AssertionError("Agent already initialized")
        self.agent = FakeManagementAgent.Agent(self.logger)
        self.agent.start()


    def stop_fake_management(self):
        if not self.agent:
            raise AssertionError("Agent not initialized")
        self.agent.stop()
        self.agent = None

    def link_appid_plugin(self, appid, pluginname):
        if not self.agent:
            raise AssertionError("Agent not initialized")
        self.appid = str(appid)
        self.pluginname = str(pluginname)
        self.agent.link_app_id_to_plugin(appid, pluginname)

    def send_policy(self, policyname ):
        policycontent = self._get_file_content(policyname)
        response = self.agent.send_apply_policy(self.appid, policycontent)
        if 'ACK' not in response:
            raise AssertionError( "Plugin Responded with error: " + str(response))

    def send_action(self, actionname ):
        actioncontent = self._get_file_content(actionname)
        response = self.agent.send_action(self.pluginname, self.appid, actioncontent)
        if 'ACK' not in response:
            raise AssertionError(str(response))


    def get_telemetry(self):
        return self.agent.get_telemetry(self.pluginname)

    def wait_registered(self):
        for i in range(10):
            if self.agent.registered_plugins:
                return
            time.sleep(0.1)
        raise AssertionError("No plugin registered recently")

    def check_status_compliant(self):
        status_contents = self.agent.get_status(self.appid, self.pluginname)
        strcontents = str(status_contents)
        self.logger.info("Current status: " + strcontents)
        if not all(['Res="Same"' in status_content.decode("utf-8") for status_content in status_contents]):
            raise AssertionError("Status content not compliant: " + strcontents)

    def check_status_contains(self, statusElement):
        status_contents = self.agent.get_status(self.appid, self.pluginname)
        strcontents = str(status_contents)
        self.logger.info("Current status: " + strcontents)
        if not all([statusElement in status_content.decode("utf-8") for status_content in status_contents]):
            raise AssertionError("Status does not contain: " + statusElement)

    def check_telemetry_contain(self, matchstring):
        self.logger.info("try to match the " + matchstring)
        content = self.get_telemetry()
        if matchstring not in content:
            raise AssertionError("Content of telemetry:" + content)

    def __del__(self):
        if self.agent:
            self.stop_fake_management()
