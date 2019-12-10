#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



import os
from robot.libraries.BuiltIn import BuiltIn


def getInstallLocation():
    env_set = os.environ.get('SOPHOS_INSTALL', None)
    if env_set:
        return env_set
    robot_set = BuiltIn().get_variable_value("${SOPHOS_INSTALL}", None)
    if robot_set:
        return robot_set
    return "/opt/sophos-spl"


INSTALL_LOCATION = getInstallLocation()
