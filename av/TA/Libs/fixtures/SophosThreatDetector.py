#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Ltd
# All rights reserved.
from __future__ import print_function, division, unicode_literals


import os
import subprocess

from . import Paths
from . import TestBase


import logging
logger = logging.getLogger("SophosThreatDetector")
logger.setLevel(logging.DEBUG)

class SophosThreatDetector(TestBase.TestBase):
    def __init__(self):
        super().__init__()
        self.__m_proc = None
        self.__m_failed = False

    def prepare_for_test(self):
        pass

    def get_log(self):
        return ""

    def start(self):
        logger.debug("SophosThreatDetector.start()")
        self.prepare_for_test()
        self.__m_proc = subprocess.Popen([Paths.sophos_threat_detector_exec_path()])
        logger.debug("Finish SophosThreatDetector.start()")

    def stop(self):
        logger.debug("SophosThreatDetector.stop()")
        if self.__m_proc:
            self.__m_proc.terminate()
            try:
                self.__m_proc.wait(15)
            except subprocess.TimeoutExpired:
                logger.fatal("Timeout attempting to terminate av plugin")
                logger.fatal("Log: %s", self.get_log())
                self.__m_proc.kill()
                self.__m_proc.wait()

            self.__m_proc = None
        if self.__m_failed:
            print("Report on Failure:")
            print(self.get_log())
        logger.debug("Finish SophosThreatDetector.stop()")

    def pid(self):
        return self.__m_proc.pid

    def log_path(self):
        logger.info("plugindir %s", str(os.listdir(Paths.av_plugin_dir())))
        logger.info("chroot %s", str(os.listdir(Paths.av_plugin_dir()+"/chroot")))
        logger.info("log %s", str(os.listdir(Paths.av_plugin_dir()+"/log")))
        logger.info("chroot/log %s", str(os.listdir(Paths.av_plugin_dir()+"/chroot/log")))
        return os.path.join(Paths.av_plugin_dir(), "chroot", "log", "sophos_threat_detector.log")
