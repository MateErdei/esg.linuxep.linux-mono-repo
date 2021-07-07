#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import subprocess

try:
    from . import ProcessUtils
except ImportError as ex:
    import ProcessUtils

from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName)) or defaultValue
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)

def get_sophos_install():
    return get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")

def get_plugin_install():
    SOPHOS_INSTALL = get_sophos_install()
    return get_variable("PLUGIN_INSTALL", os.path.join(SOPHOS_INSTALL, "plugins", "av"))


def install_av_if_not_installed():
    PLUGIN_INSTALL = get_plugin_install()
    if not os.path.isdir(PLUGIN_INSTALL):
        BuiltIn().run_keyword("Install AV Directly from SDDS")

def __start_plugin(name):
    wdctl = os.path.join(get_sophos_install(), "bin", "wdctl")
    assert os.path.isfile(wdctl)
    subprocess.check_call([wdctl, "start", name])

def start_av_plugin_if_not_running():
    PLUGIN_INSTALL = get_plugin_install()
    av_exe = os.path.join(PLUGIN_INSTALL, "sbin", "av")
    pid = ProcessUtils.pidof(av_exe)
    if pid == -1:
        __start_plugin("av")
        ProcessUtils.wait_for_pid(av_exe, 15)

    av_log = os.path.join(PLUGIN_INSTALL, "log", "av.log")
    BuiltIn().run_keyword("Wait Until File exists", av_log)

def start_sophos_threat_detector_if_not_running():
    PLUGIN_INSTALL = get_plugin_install()
    threat_detector_exe = os.path.join(PLUGIN_INSTALL, "sbin", "sophos_threat_detector")
    pid = ProcessUtils.pidof(threat_detector_exe)
    if pid == -1:
        __start_plugin("threat_detector")
        pid = ProcessUtils.wait_for_pid(threat_detector_exe, 15)

    threat_detector_log = os.path.join(PLUGIN_INSTALL, "chroot", "log", "sophos_threat_detector.log")
    BuiltIn().run_keyword("Wait Until File exists", threat_detector_log)
    return pid