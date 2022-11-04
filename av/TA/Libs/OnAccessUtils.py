#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import six
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

try:
    from . import LogHandler
except ImportError:
    import LogHandler


class OnAccessUtils:
    @staticmethod
    def wait_for_on_access_to_be_enabled(mark: LogHandler.LogMark, timeout=15):
        """
        Replaces
        Wait Until On Access Log Contains With Offset   On-open event for

        :return:
        """
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")
        mark.assert_is_good(on_access_log)

        start = time.time()
        log_contents = []
        while time.time() < start + timeout:
            log_contents = mark.get_contents().splitlines()
            log_contents = [  six.ensure_text(line, "UTF-8") for line in log_contents ]
            for line in log_contents:
                if u"On-open event for" in line:
                    logger.info(u"On Access enabled with " + line)
                    return

                if u"On-close event for" in line:
                    logger.info(u"On Access enabled with " + line)
                    return

            time.sleep(0.1)
            open("/etc/hosts")  # trigger some activity

        raise AssertionError(u"On-Access not enabled within %d seconds: Logs: %s" % (timeout, u"\n".join(log_contents)))
