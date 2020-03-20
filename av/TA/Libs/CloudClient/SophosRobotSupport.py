#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os

from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError


def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName), defaultValue)
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)


def TMPROOT():
    return get_variable("TMPROOT", "/tmp")
