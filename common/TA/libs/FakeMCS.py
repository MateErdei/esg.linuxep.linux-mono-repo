#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2023 Sophos Limited. All rights reserved.

import os

import PathManager

"""
keywords to talk to management agent, pretending to be mcsrouter
send actions and policies
receive events and statuses
"""


def send_action(src_file_name, dest_file_name, **replacements):
    contents = open(src_file_name).read()
    if len(contents) == 0:
        raise AssertionError(f"{src_file_name} is empty!")

    for k, v in replacements.items():
        contents = contents.replace(k, v)
    tmp_name = os.path.join(PathManager.SOPHOS_INSTALL, "tmp", "TempAction.xml")
    open(tmp_name, "w").write(contents)
    os.rename(tmp_name, dest_file_name)
