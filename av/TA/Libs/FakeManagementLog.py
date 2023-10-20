#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys


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
except ImportError:
    try:
        from Libs.PluginCommunicationTools import FakeManagementAgent
    except ImportError:
        from PluginCommunicationTools import FakeManagementAgent


def get_fake_management_log_filename():
    return "fake_management_agent.log"


def get_fake_management_log_path():
    return os.path.join(FakeManagementAgent.get_log_dir(), get_fake_management_log_filename())
