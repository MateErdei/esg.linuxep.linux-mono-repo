#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Ltd
# All rights reserved.
from __future__ import print_function, division, unicode_literals

import os
import time

from . import Paths

def file_content(path):
    with open(path, 'r') as f:
        return f.read()

class TestBase(object):
    def get_log(self):
        return ""

    def wait_file(self, relative_path, timeout=10):
        full_path = os.path.join(Paths.sophos_spl_path(), relative_path)
        for i in range(timeout):
            if os.path.exists(full_path):
                return file_content(full_path)
            time.sleep(1)
        dir_path = os.path.dirname(full_path)
        files_in_dir = os.listdir(dir_path)
        raise AssertionError("File not found after {} seconds. Path={}.\n Files in Directory {} \n Log:\n {}".
                             format(timeout, full_path, files_in_dir, self.get_log()))
