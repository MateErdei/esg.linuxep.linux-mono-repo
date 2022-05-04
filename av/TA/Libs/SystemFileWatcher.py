#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

SYSTEM_FILES = [
    "nsswitch.conf",
    "resolv.conf",
    "ld.so.cache",
    "host.conf",
    "hosts",
]

class SystemFileWatcher(object):
    def __init__(self):
        self.__m_file_stat = None

    def __stat(self):
        ret = {}
        for f in SYSTEM_FILES:
            p = os.path.join("/etc", f)
            ret[f] = os.stat(p)
        return ret

    def start_watching_system_files(self):
        self.__m_file_stat = self.__stat()

    def stop_watching_system_files(self):
        current_stat = self.__stat()
        for f in SYSTEM_FILES:
            current = current_stat[f]
            old = self.__m_file_stat[f]
            if current.st_mtime != old.st_mtime:
                current_mtime = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(current.st_mtime))
                logger.error("System file changed while being watched: %s at %s" % (f, current_mtime))
