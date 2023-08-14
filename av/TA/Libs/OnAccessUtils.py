#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import re
import six
import subprocess
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

try:
    from . import LogHandler
except ImportError:
    import LogHandler


class OnAccessUtils:
    @staticmethod
    def wait_for_on_access_to_be_enabled(mark: LogHandler.LogMark, timeout=15, file="/etc/hosts", log_path=None):
        """
        Replaces
        Wait Until On Access Log Contains With Offset   On-open event for

        :return:
        """
        on_access_log = log_path if log_path else BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")
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
            open(file)  # trigger some activity

        raise AssertionError(u"On-Access not enabled within %d seconds: Logs: %s" % (timeout, u"\n".join(log_contents)))

    @staticmethod
    def check_multiple_on_access_threads_are_scanning(mark: LogHandler.LogMark, min_number_of_scanners=5, timeout=60):
        on_access_log = BuiltIn().get_variable_value("${ON_ACCESS_LOG_PATH}")
        mark.assert_is_good(on_access_log)

        start = time.time()
        log_contents = []
        unique_threads = set()
        regex = re.compile(r"ms by scanHandler-(\d+)")
        while time.time() < start + timeout:
            log_contents = mark.get_contents().splitlines()
            log_contents = [six.ensure_text(line, "UTF-8") for line in log_contents]
            for line in log_contents:
                regex_result = regex.search(line)
                if regex_result:
                    handler_id = int(regex_result.group(1))
                    unique_threads.add(handler_id)

            if len(unique_threads) >= min_number_of_scanners:
                logger.info(u"Minimum requirement satisfied thread ids found: " + str(unique_threads))
                return
            time.sleep(0.5)

        raise AssertionError(u"On-Access did not use enough scanners within %d seconds: Logs: %s" % (timeout, u"\n".join(log_contents)))

    @staticmethod
    def wait_for_on_access_enabled_by_status_file(timeout=15):
        start = time.time()
        var_dir = BuiltIn().get_variable_value("${COMPONENT_VAR_DIR}")
        status_file = os.path.join(var_dir, "on_access_status")
        while time.time() < start + timeout:
            with open(status_file) as f:
                value = f.read(1)
                if value == "e":
                    return True
        raise AssertionError("On-Access status file didn't report enabled within timeout")

    @staticmethod
    def get_last_line_from_mount_ignoring_tmpfs():
        result = subprocess.run(["mount"], check=True, stdout=subprocess.PIPE)
        output = result.stdout.decode("UTF-8")
        lines = output.splitlines()
        lines.reverse()
        for line in lines:
            if line.startswith("tmpfs on "):
                continue
            return line


