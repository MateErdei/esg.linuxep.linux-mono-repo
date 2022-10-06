#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import subprocess
import threading
import time

from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
from robot.api import logger

def get_variable(varName, defaultValue=None):
    try:
        return BuiltIn().get_variable_value("${}".format(varName), defaultValue)
    except RobotNotRunningError:
        return os.environ.get(varName, defaultValue)

def safe_delete(p):
    try:
        os.unlink(p)
    except EnvironmentError:
        pass

class FakeWatchdogThread(object):
    def __init__(self):
        self.__m_log_history = []
        self.__m_stop_event = threading.Event()
        self.__m_command = [get_variable("SOPHOS_THREAT_DETECTOR_LAUNCHER")]
        self.__m_stdout_path = "/tmp/threat_detector.stdout"
        self.__m_stderr_path = "/tmp/threat_detector.stderr"
        self.__m_proc = None
        safe_delete(self.__m_stdout_path)
        safe_delete(self.__m_stderr_path)
        self.__m_expect_start_failure = False

    def expect_start_failure(self):
        self.__m_expect_start_failure = True


    def info(self, msg):
        self.__m_log_history.append(("INFO", msg))

    def warn(self, msg):
        self.__m_log_history.append(("WARN", msg))

    def set_command(self, command):
        self.__m_command = command

    def start_process(self):
        """
        Start Process  ${SOPHOS_THREAT_DETECTOR_LAUNCHER}   stdout=/tmp/threat_detector.stdout  stderr=/tmp/threat_detector.stderr
        :return:
        """
        if not self.__m_expect_start_failure:
            self.info("Starting process")

        if self.__m_proc is not None:
            self.stop_process()

        try:
            self.__m_proc = subprocess.Popen(self.__m_command,
                                             stdout=open(self.__m_stdout_path, "w"),
                                             stderr=open(self.__m_stderr_path, "w"))
            return 0
        except EnvironmentError as ex:
            if not self.__m_expect_start_failure:
                self.warn(time.strftime("%H:%M:%S")+": Unable to start process: "+str(ex))
            return 10

    def stop_process(self):
        if self.__m_proc is None:
            return -1

        if self.process_is_running():
            self.info("Stopping process")
            self.__m_proc.kill()
        ret = self.__m_proc.wait()
        self.__m_proc = None
        return ret

    def process_is_running(self):
        if self.__m_proc is None:
            return False
        return self.__m_proc.poll() is None

    def __call__(self):
        # run thread
        # start process
        self.start_process()
        # check for termination, or stop request
        while True:
            if self.__m_stop_event.is_set():
                break
            if not self.process_is_running():
                if self.__m_proc is not None:
                    return_code = self.__m_proc.wait()
                    if return_code < 0:
                        # A negative value -N indicates that the child was terminated by signal N
                        self.warn("Restarting Sophos Threat Detector! Killed by signal = %d" % -return_code)
                    else:
                        self.warn("Restarting Sophos Threat Detector! Exit code = %d" % return_code)
                sleep = self.start_process()
                time.sleep(sleep)
            time.sleep(0.1)
        # stop process
        self.stop_process()

    def log_history(self):
        """
        Log items from history in the right thread
        :return:
        """
        for (level, item) in self.__m_log_history:
            logger.write(item, level)

    def attemptStop(self):
        self.__m_stop_event.set()

class FakeWatchdog(object):
    ROBOT_LIBRARY_SCOPE = "SUITE"

    def __init__(self):
        self.__m_td_thread = None
        self.__m_td_worker = None

    def start_sophos_threat_detector_under_fake_watchdog(self):
        if self.__m_td_thread is not None:
            self.stop_sophos_threat_detector_under_fake_watchdog()
            logger.error("Trying to start Threat Detector while it is running!")

        self.__m_td_worker = FakeWatchdogThread()
        self.__m_td_thread = threading.Thread(target=self.__m_td_worker)
        self.__m_td_thread.start()

    def stop_sophos_threat_detector_under_fake_watchdog(self):
        if self.__m_td_thread is not None:
            self.__m_td_worker.attemptStop()
            self.__m_td_thread.join()
            self.__m_td_worker.log_history()
            self.__m_td_thread = None
            self.__m_td_worker = None

    def dump_log_history(self):
        if self.__m_td_worker is not None:
            self.__m_td_worker.log_history()

    def expect_start_failure(self):
        if self.__m_td_worker is not None:
            self.__m_td_worker.expect_start_failure()



