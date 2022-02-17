#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.
import re

from robot.api import logger


def version_ini_file_contains_proper_format(file):
    name_pattern = "^PRODUCT_NAME = Sophos Server Protection Linux - av\n\Z"
    version_pattern = "^PRODUCT_VERSION = ([0-9]*\.)*[0-9]*\n\Z"
    date_pattern = "^BUILD_DATE = [0-9]{4}\-[0-9]{2}\-[0-9]{2}\n\Z"
    git_commit_pattern = "^COMMIT_HASH = [0-9a-fA-F]{40}\n\Z"
    plugin_api_commit_pattern = "^PLUGIN_API_COMMIT_HASH = [0-9a-fA-F]{40}\n\Z"

    patterns = [name_pattern, version_pattern, date_pattern, git_commit_pattern, plugin_api_commit_pattern]

    lines = []
    with open(file) as f:
        lines = f.readlines()

    if len(lines) != len(patterns):
        raise AssertionError

    n = 0
    for line in lines:
        if re.match(patterns[n], line) is None:
            logger.info(f"line = ||{line}||")
            logger.info(f"{line} does not match {patterns[n]}")
            raise AssertionError
        n += 1
