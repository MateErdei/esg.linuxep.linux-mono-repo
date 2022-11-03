#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import six
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger


class OnAccessUtils:
    @staticmethod
    def wait_for_on_access_to_be_enabled(mark, timeout=15):
        """
        Replaces
        Wait Until On Access Log Contains With Offset   On-open event for

        :return:
        """
        logutils = BuiltIn().get_library_instance("LogUtils")
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")

        start = time.time()
        handler = logutils.get_log_handler(on_access_log)
        handler.assert_mark_is_good(mark)
        log_contents = []
        while time.time() < start + timeout:
            log_contents = handler.get_contents(mark).splitlines()
            log_contents = [  six.ensure_text(line, "UTF-8") for line in log_contents  ]
            for line in log_contents:
                if u"On-open event for" in line:
                    logger.info(u"On Access enabled with " + line)
                    return

                if u"On-close event for" in line:
                    logger.info(u"On Access enabled with " + line)
                    return

            time.sleep(0.1)

        raise AssertionError(u"On-Access not enabled within %d seconds: Logs: %s" % (timeout, "".join(log_contents)))
