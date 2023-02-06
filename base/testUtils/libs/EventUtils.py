#!/usr/bin/env python3

import os

from robot.api import logger


def check_at_least_one_event_has_substr(event_dir, expected_contents):
    for f in os.listdir(event_dir):
        path = os.path.join(event_dir, f)
        contents = open(path).read()
        if expected_contents in contents:
            logger.info("Found %s in %s" % (expected_contents, path))
            return

    raise AssertionError("Failed to find %s in an events in %s" % (expected_contents, event_dir))
