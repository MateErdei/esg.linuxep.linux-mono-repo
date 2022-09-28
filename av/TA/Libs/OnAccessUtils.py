#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger


class OnAccessUtils:
    @staticmethod
    def wait_for_on_access_to_be_enabled(timeout=15):
        """
        Replaces
        Wait Until On Access Log Contains With Offset   On-open event for

        :return:
        """
        # logutils = BuiltIn().get_library_instance("LogUtils")
        mark = BuiltIn().get_variable_value("${ON_ACCESS_LOG_MARK}", 0)
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")

        start = time.time()
        log_contents = []
        while time.time() < start + timeout:
            log_contents = open(on_access_log).readlines()
            log_contents = log_contents[mark:]
            for line in log_contents:
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

        raise AssertionError("On-Access not enabled within %d seconds: Logs: %s" % (timeout, "\n".join(log_contents)))
