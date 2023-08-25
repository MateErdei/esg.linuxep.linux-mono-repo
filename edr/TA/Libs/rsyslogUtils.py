#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2023 Sophos Ltd
# All rights reserved.


import subprocess

from robot.api import logger


def require_rsyslog():
    """
    Duplicate systemctl list-unit-files | grep -q rsyslog
    Raise Exception if rsyslog not available
    :return:
    """
    output = subprocess.check_output(["systemctl", "list-unit-files"])
    output = output.decode('UTF-8')
    logger.debug(f"list-unit-files: {output}")
    if "rsyslog" in output:
        return
    logger.info(f"rsyslog not in: {output}")

    output = subprocess.run(['rpm', '-qa'], stdout=subprocess.PIPE).stdout
    logger.info(f"rpm -qa: {output}")

    output = subprocess.run(['dpkg', '-l'], stdout=subprocess.PIPE).stdout
    logger.info(f"dpkg -l: {output}")

    raise AssertionError("rsyslog not available: not listed in systemctl list-unit-files")
