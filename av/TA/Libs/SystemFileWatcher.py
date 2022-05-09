#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import difflib
import hashlib
import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.libraries.BuiltIn import RobotNotRunningError
from robot.api import logger

SYSTEM_FILES = [
    "nsswitch.conf",
    "resolv.conf",
    "ld.so.cache",
    "host.conf",
    "hosts",
]

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName), defaultValue)
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)

def _ensure_unicode(s):
    if isinstance(s, bytes):
        try:
            return s.decode("UTF-8")
        except UnicodeDecodeError:
            return s.decode("Latin-1")
    return s

class SystemFileWatcher(object):
    def __init__(self):
        self.__m_file_stat = None
        self.__m_file_contents = None

    def __stat(self):
        ret = {}
        for f in SYSTEM_FILES:
            p = os.path.join("/etc", f)
            ret[f] = os.stat(p)
        return ret

    def __get_contents(self):
        ret = {}
        for f in SYSTEM_FILES:
            p = os.path.join("/etc", f)
            ret[f] = open(p, "rb").read()
        return ret

    def start_watching_system_files(self):
        self.__m_file_stat = self.__stat()
        self.__m_file_contents = self.__get_contents()

    def stop_watching_system_files(self):
        current_stat = self.__stat()
        current_contents = self.__get_contents()
        any_changed = False
        for f in SYSTEM_FILES:
            contents_diff = False
            contents = current_contents[f]
            old_contents = self.__m_file_contents[f]
            if contents != old_contents:
                contents = _ensure_unicode(contents).splitlines()
                old_contents = _ensure_unicode(old_contents).splitlines()
                diff = list(difflib.unified_diff(old_contents, contents))
                logger.error("%s changed contents while being watched: %s" % (f, diff))
                contents_diff = True
                any_changed = True

            current = current_stat[f]
            old = self.__m_file_stat[f]
            if current.st_mtime != old.st_mtime:
                current_mtime = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(current.st_mtime))
                if contents_diff:
                    logger.error("%s changed mtime while being watched at %s" % (f, current_mtime))
                else:
                    logger.error("%s changed mtime without changing contents while being watched at %s" %
                                 (f, current_mtime))
                any_changed = True

        if any_changed:
            #  See if we've got anything in av log about this
            BuiltIn().run_keyword("Dump Log", get_variable("AV_LOG_PATH"))
