#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.
import os


def get_install_location():
    env_set = os.environ.get('SOPHOS_INSTALL', None)
    if env_set:
        return env_set
    return "/opt/sophos-spl"


def ipc_dir():
    ipc_dir = os.path.join(get_install_location(), "var/ipc")
    print('inside ipc_dir: {}'.format(ipc_dir))
    return ipc_dir


def management_agent_socket_path():
    return "ipc://{}".format(os.path.join(ipc_dir(), "mcs_agent.ipc"))

