#!/usr/bin/env python3
# Copyright 2023 Sophos Limited. All rights reserved.

import time

from robot.api import logger


def wait_for_file_to_contain(path, expected, timeout=30):
    contents = "NEVER READ!"
    timelimit = time.time() + timeout
    while time.time() < timelimit:
        contents = open(path).read()
        if expected in contents:
            logger.info("Found %s in %s: %s" % (expected, path, contents))
            return
        time.sleep(0.5)
    raise AssertionError("Failed to find %s in %s: last contents: %s" % (expected, path, contents))

