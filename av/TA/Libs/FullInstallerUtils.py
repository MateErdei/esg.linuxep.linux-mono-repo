#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2022-2023 Sophos Limited. All rights reserved.

import re

from robot.api import logger


def version_ini_file_contains_proper_format(file):
    name_pattern = r"^PRODUCT_NAME = SPL-Anti-Virus-Plugin\n\Z"
    version_pattern = r"^PRODUCT_VERSION = ([0-9]*\.)*[0-9]*\n\Z"
    date_pattern = r"^BUILD_DATE = [0-9]{4}\-[0-9]{2}\-[0-9]{2}\n\Z"
    git_commit_pattern = r"^COMMIT_HASH = [0-9a-fA-F]{40}\n\Z"

    patterns = [name_pattern, version_pattern, date_pattern, git_commit_pattern]

    lines = []
    with open(file) as f:
        lines = f.readlines()

    if len(lines) < len(patterns):
        raise AssertionError(f"Insufficient lines in VERSION.ini {file}")

    for n in range(len(patterns)):
        pattern = patterns[n]
        line = lines[n]
        if re.match(pattern, line) is None:
            logger.error(f"line = ||{line}||")
            raise AssertionError(f"{line} from {file} does not match {pattern}")
