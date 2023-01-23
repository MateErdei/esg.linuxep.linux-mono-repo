#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2021-2023 Sophos Limited. All rights reserved.
# All rights reserved.

import os
import subprocess

try:
    from . import LogUtils
    from . import ProcessUtils
except ImportError as ex:
    import LogUtils
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
    assert os.path.isfile(wdctl), "Cannot find wdctl"
    subprocess.check_call([wdctl, "start", name])


def start_av_plugin_if_not_running():
    PLUGIN_INSTALL = get_plugin_install()
    av_exe = os.path.join(PLUGIN_INSTALL, "sbin", "av")
    pid = ProcessUtils.pidof(av_exe)
    if pid == -1:
        logUtility = LogUtils.LogUtils()
        mark = logUtility.get_av_log_mark()
        __start_plugin("av")
        ProcessUtils.wait_for_pid(av_exe, 15)
        BuiltIn().run_keyword("Wait until AV Plugin running after mark", mark)
    return pid


def start_on_access_if_not_running():
    PLUGIN_INSTALL = get_plugin_install()
    oa_exe = os.path.join(PLUGIN_INSTALL, "sbin", "soapd")
    pid = ProcessUtils.pidof(oa_exe)
    if pid == -1:
        logUtility = LogUtils.LogUtils()
        mark = logUtility.get_on_access_log_mark()
        __start_plugin("soapd")
        ProcessUtils.wait_for_pid(oa_exe, 15)
        BuiltIn().run_keyword("Wait Until On Access running after mark", mark)
    return pid

def start_sophos_threat_detector_if_not_running():
    PLUGIN_INSTALL = get_plugin_install()
    threat_detector_exe = os.path.join(PLUGIN_INSTALL, "sbin", "sophos_threat_detector")
    pid = ProcessUtils.pidof(threat_detector_exe)
    if pid == -1:
        logUtility = LogUtils.LogUtils()
        mark = logUtility.get_sophos_threat_detector_log_mark()
        __start_plugin("threat_detector")
        pid = ProcessUtils.wait_for_pid(threat_detector_exe, 15)

        BuiltIn().run_keyword("Wait until threat detector running after mark", mark)
    return pid
