#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger


class OnAccessUtils:
    def wait_for_on_access_to_be_enabled(self, timeout=15):
        """
        Replaces
        Wait Until On Access Log Contains With Offset   On-open event for

        :return:
        """
        logutils = BuiltIn().get_library_instance("LogUtils")
        mark = BuiltIn().get_variable_value("${ON_ACCESS_LOG_MARK}", 0)
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")

        start = time.time()
        logContents = []
        while time.time() < start + timeout:
            logContents = open(on_access_log).readlines()
            logContents = logContents[mark:]
            for line in logContents:
                if "On-open event for" in line:
                    logger.info("On Access enabled with " + line)
                    return

                if "On-close event for" in line:
                    logger.info("On Access enabled with " + line)
                    return

            time.sleep(0.1)
            tempfile = "/tmp/%f" % time.time()
            open(tempfile, "w")
            os.unlink(tempfile)

        raise AssertionError("On-Access not enabled within %d seconds: Logs: %s" % (timeout, "\n".join(logContents)))
