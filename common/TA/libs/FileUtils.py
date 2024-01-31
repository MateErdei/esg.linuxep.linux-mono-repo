#!/usr/bin/env python3
# Copyright 2023 Sophos Limited. All rights reserved.

import time
import zipfile

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


def check_if_zip_file_is_password_protected(path):
    with zipfile.ZipFile(path) as z:
        for i in z.infolist():
            try:
                with z.open(i) as o:
                    return False
                    pass
            except RuntimeError:
                return True


def assert_zip_file_is_password_protected(path):
    encrypted = check_if_zip_file_is_password_protected(path)
    if not encrypted:
        raise AssertionError(f"Able to open file from {path} without a password")
    logger.debug(f"{path} is password protected")


if __name__ == "__main__":
    import sys
    assert_zip_file_is_password_protected(sys.argv[1])
