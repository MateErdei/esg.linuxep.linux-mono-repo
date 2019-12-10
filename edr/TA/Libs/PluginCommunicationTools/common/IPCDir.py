#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2018-2019 Sophos Plc, Oxford, England.
# All rights reserved.



from . import InstallLocation

def ipc_dir():
    return "{}/var/ipc".format(InstallLocation.getInstallLocation())
