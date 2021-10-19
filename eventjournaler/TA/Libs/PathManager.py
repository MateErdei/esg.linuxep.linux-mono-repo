#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os

try:
    from robot.api import logger
except:
    import logging

    logger = logging.getLogger(__name__)

THIS_FILE_PATH = os.path.realpath(__file__)
SOPHOS_INSTALL = os.path.join("/", "opt", "sophos-spl")


def are_basenames_in_directory(directory_to_check, basenames, callback_to_verify_paths=None):
    """

    :param directory_to_check: directory to look for things in
    :param basenames: things to look for in that directory
    :param callback_to_verify_paths: function to pass the paths being looked for to for verification
    (e.g. os.path.isdir)
    :return: True if all basenames exist in the directory_to_check
    """
    if isinstance(basenames, str):
        basenames = [basenames]
    for basename in basenames:
        path_to_check = os.path.join(directory_to_check, basename)
        if not os.path.exists(path_to_check):
            return False
        if callback_to_verify_paths:
            if not callback_to_verify_paths(path_to_check):
                raise AssertionError("{} exists but did not return true when passed to {}".format(path_to_check,
                                                                                                  callback_to_verify_paths))
    return True


def libs_supportfiles_and_tests_are_here(dir_path):
    return are_basenames_in_directory(dir_path, ["Libs", "RobotTests"])


def get_testUtils_dir():
    dir_path = os.path.dirname(THIS_FILE_PATH)
    # go up the directory structure until we have the right directory
    while not libs_supportfiles_and_tests_are_here(dir_path):
        dir_path = os.path.dirname(dir_path)
        if dir_path == "/":
            raise AssertionError("Failed to find testUtils dir, recursed till reached root")
    return dir_path


ROBOT_ROOT_PATH = get_testUtils_dir()
ROBOT_TESTS_PATH = os.path.join(ROBOT_ROOT_PATH, "RobotTests")


def get_robot_tests_path():
    return ROBOT_TESTS_PATH
