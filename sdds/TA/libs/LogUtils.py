#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021 Sophos Plc, Oxford, England.
# All rights reserved.


import os
import glob
import subprocess
import dateutil.parser
from robot.api import logger
import robot.libraries.BuiltIn


def get_log_contents(path_to_log):
    with open(path_to_log, "r") as log:
        contents = log.read()
    return contents


def get_variable(varName, defaultValue=None):
    try:
        var = robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % varName)
        if var is not None:
            return var
    except robot.libraries.BuiltIn.RobotNotRunningError:
        pass

    return os.environ.get(varName, defaultValue)


class LogUtils(object):
    def __init__(self):
        self.tmp_path = os.path.join(".", "tmp")
        self.install_path = get_variable("SOPHOS_INSTALL", os.path.join("/", "opt", "sophos-spl"))
        assert self.install_path is not None
        self.base_logs_dir = get_variable("BASE_LOGS_DIR", os.path.join(self.install_path, "logs", "base"))
        assert self.base_logs_dir is not None

    def dump_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "r") as f:
                    logger.info("file: " + str(filename))
                    logger.info(''.join(f.readlines()))
            except Exception as e:
                logger.info("Failed to read file: {}".format(e))
        else:
            logger.info("File does not exist")

    def check_log_contains(self, string_to_contain, pathToLog, log_name):
        if not (os.path.isfile(pathToLog)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, pathToLog))

        contents = get_log_contents(pathToLog)
        if string_to_contain not in contents:
            self.dump_log(pathToLog)
            raise AssertionError("{} Log at \"{}\" does not contain: {}".format(log_name, pathToLog, string_to_contain))

    def mcs_router_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")

    def check_mcsrouter_log_contains(self, string_to_contain):
        mcsrouter_log = self.mcs_router_log()
        self.check_log_contains(string_to_contain, mcsrouter_log, "MCS Router")
        logger.info(mcsrouter_log)
