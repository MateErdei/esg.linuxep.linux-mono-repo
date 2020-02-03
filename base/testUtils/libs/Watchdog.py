#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import json
import os
import subprocess

import signal
import time

from robot.api import logger


def get_install():
    return os.environ.get("SOPHOS_INSTALL","/opt/sophos-spl")


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
            logger.debug("Watchdog exited with %d", code)
            self.m_watchdog_process = None

    def setup_test_plugin_config(self, cmd, dest=None, pluginName=None, pluginFileName="testPlugin.sh"):
        testPluginExe = os.path.join(get_install(), pluginFileName)
        open(testPluginExe, "w").write("#!/bin/bash\n"+cmd+"\n")
        subprocess.check_call(["chmod", "0700", testPluginExe])
        if dest is None:
            dest = os.path.join(get_install(), "base", "pluginRegistry")
        if pluginName is None:
            pluginName = "fakePlugin"

        testPluginRegistry = os.path.join(dest, pluginName+".json")
        data = {
            "executableFullPath": testPluginExe,
            "pluginName": pluginName,
            "executableUserAndGroup": "root:root"
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

        raise AssertionError("%s not created in %f seconds"%(p,duration))

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

        raise AssertionError("Plugin process not restarted in %f seconds" % duration)

    def kill_plugin(self, pidfile):
        lines = open(pidfile).readlines()
        pid = int(lines[-1].strip())
        os.kill(pid, signal.SIGTERM)
