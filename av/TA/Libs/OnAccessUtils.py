#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import re

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
            log_contents = [six.ensure_text(line, "UTF-8") for line in log_contents]
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

    @staticmethod
    def check_multiple_on_access_threads_are_scanning(mark: LogHandler.LogMark, timeout=60):
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")
        mark.assert_is_good(on_access_log)

        min_number_of_scanners = 5
        start = time.time()
        log_contents = []
        unique_threads = set()
        while time.time() < start + timeout:
            log_contents = mark.get_contents().splitlines()
            log_contents = [six.ensure_text(line, "UTF-8") for line in log_contents]
            for line in log_contents:
                regex = re.compile(r"ms by scanHandler-(\d+)")
                regex_result = regex.search(line)
                if regex_result:
                    handler_id = int(regex_result.group(1))
                    unique_threads.add(handler_id)

            if len(unique_threads) >= min_number_of_scanners:
                logger.info(u"Minimum requirement satisfied thread ids found: " + str(unique_threads))
                return

        raise AssertionError(u"On-Access did not use enough scanners within %d seconds: Logs: %s" % (timeout, u"\n".join(log_contents)))

