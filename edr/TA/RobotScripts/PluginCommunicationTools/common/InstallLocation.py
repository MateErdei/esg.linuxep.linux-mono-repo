#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



import os
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError


def getInstallLocation():
    try:
        return os.environ.get('SOPHOS_INSTALL', None) or BuiltIn().get_variable_value("${SOPHOS_INSTALL}")
    except RobotNotRunningError:
        return "/opt/sophos-spl"


INSTALL_LOCATION = getInstallLocation()
