#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import sys
import errno


def appendPath(d):
    if d not in sys.path:
        sys.path.append(d)


def get_modules_dir():
    UNIT_TEST_DIR = os.path.dirname(__file__)
    TEST_DIR = os.path.dirname(UNIT_TEST_DIR)
    SOURCE_DIR = os.path.dirname(TEST_DIR)
    MODULES_DIR = os.path.join(SOURCE_DIR, "modules")
    return MODULES_DIR

def get_mcs_router_dir():
    return os.path.join(get_modules_dir(), "mcsrouter")

def safeDelete(path):
    try:
        os.unlink(path)
    except EnvironmentError as e:
        if e.errno == errno.ENOENT:
            return
        else:
            raise

appendPath(get_mcs_router_dir())
