#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2023 Sophos Plc, Oxford, England.
# All rights reserved.
import grp
import json
import os
import pwd
import shutil
import subprocess

import signal
import time
from pwd import getpwnam

from robot.api import logger


def get_install():
    return os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")


class Watchdog(object):
    def __init__(self):
        self.m_watchdog_process = None

    def manually_start_watchdog(self):
        INST = get_install()
        watchdogExe = os.path.join(INST, "base", "bin", "sophos_watchdog")
        env = os.environ.copy()
        env['SOPHOS_INSTALL'] = INST
        logger.info("Starting manual watchdog")
        self.m_watchdog_process = subprocess.Popen([watchdogExe], cwd=INST, env=env)

    def kill_manual_watchdog(self):
        if self.m_watchdog_process is not None:
            logger.info("Terminating manual watchdog")
            self.m_watchdog_process.terminate()
            code = self.m_watchdog_process.wait()
            logger.debug(f"Watchdog exited with {code}")
            self.m_watchdog_process = None

    def setup_test_plugin_config(self, cmd, dest=None, pluginName=None, pluginFileName="testPlugin.sh"):
        testPluginExe = os.path.join(get_install(), pluginFileName)
        open(testPluginExe, "w").write("#!/bin/bash\n" + cmd + "\n")
        subprocess.check_call(["chmod", "0700", testPluginExe])
        if dest is None:
            dest = os.path.join(get_install(), "base", "pluginRegistry")
        if pluginName is None:
            pluginName = "fakePlugin"

        testPluginRegistry = os.path.join(dest, pluginName + ".json")
        data = {
            "executableFullPath": testPluginExe,
            "pluginName": pluginName,
            "executableUserAndGroup": "root:root"
        }
        open(testPluginRegistry, "w").write(json.dumps(data))

    def setup_test_plugin_config_with_given_executable(self, path, pluginName=None, pluginFileName="testPlugin"):
        testPluginExe = os.path.join(get_install(), pluginFileName)
        shutil.copy(path, testPluginExe)
        subprocess.check_call(["chmod", "0700", testPluginExe])

        dest = os.path.join(get_install(), "base", "pluginRegistry")
        if pluginName is None:
            pluginName = "fakePlugin"

        testPluginRegistry = os.path.join(dest, pluginName + ".json")
        data = {
            "executableFullPath": testPluginExe,
            "pluginName": pluginName,
            "executableUserAndGroup": "root:root",
            "secondsToShutDown": 5
        }
        open(testPluginRegistry, "w").write(json.dumps(data))

    def wait_for_marker_to_be_created(self, p, duration):
        duration = float(duration)
        start = time.time()
        end = start + duration
        while end > time.time():
            if os.path.isfile(p):
                return
            time.sleep(1)

        raise AssertionError(f"{p} not created in {duration} seconds")

    def wait_for_plugin_to_start(self, p, duration):
        return self.wait_for_marker_to_be_created(p, duration)

    def wait_for_plugin_to_start_again(self, p, duration):
        """
        :param p:
        :param duration:
        :return: Assert if we don't detect a restart in time
        """
        duration = float(duration)
        start = time.time()
        end = start + duration
        while end > time.time():
            lines = open(p).readlines()
            if len(lines) >= 2:
                return
            time.sleep(0.5)

        raise AssertionError(f"Plugin process not restarted in {duration} seconds")

    def kill_plugin(self, pidfile):
        lines = open(pidfile).readlines()
        pid = int(lines[-1].strip())
        os.kill(pid, signal.SIGTERM)

    def verify_watchdog_config(self, expect_all_users=False):
        with open(os.path.join(get_install(), "base", "etc", "user-group-ids-actual.conf")) as config_file:
            watchdog_config = json.loads(config_file.read())

        logger.info(f"Actual User/Group ID Config: {watchdog_config}")

        if expect_all_users:
            expected_config = {
                "groups": {
                    "sophos-spl-group": grp.getgrnam("sophos-spl-group").gr_gid,
                    "sophos-spl-ipc": grp.getgrnam("sophos-spl-ipc").gr_gid
                },
                "users": {
                    "sophos-spl-av": pwd.getpwnam("sophos-spl-av").pw_uid,
                    "sophos-spl-local": pwd.getpwnam("sophos-spl-local").pw_uid,
                    "sophos-spl-threat-detector": pwd.getpwnam("sophos-spl-threat-detector").pw_uid,
                    "sophos-spl-updatescheduler": pwd.getpwnam("sophos-spl-updatescheduler").pw_uid,
                    "sophos-spl-user": pwd.getpwnam("sophos-spl-user").pw_uid
                }
            }
        else:
            expected_config = {"groups": {}, "users": {}}

            for group_data in grp.getgrall():
                group = group_data[0]
                if group.startswith("sophos-spl"):
                    gid = grp.getgrnam(group)[2]
                    expected_config["groups"][group] = gid

            for user_data in pwd.getpwall():
                user = user_data[0]
                if user.startswith("sophos-spl"):
                    uid = getpwnam(user).pw_uid
                    expected_config["users"][user] = uid

        if expected_config != watchdog_config:
            raise AssertionError(f"Watchdog config does not match expected\n"
                                 f"Expected config: {expected_config}\n"
                                 f"Actual config: {watchdog_config}")
