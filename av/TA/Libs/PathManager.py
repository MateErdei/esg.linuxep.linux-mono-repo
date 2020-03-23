#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys
try:
    from robot.api import logger
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

THIS_FILE_PATH = os.path.realpath(__file__)


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
                raise AssertionError("{} exists but did not return true when passed to {}".format(path_to_check, callback_to_verify_paths))
    return True


def _expected_directories_are_here(dir_path):
    return are_basenames_in_directory(dir_path, ["Libs", "component", "integration", "resources"])


def get_TA_dir():
    dir_path = os.path.dirname(THIS_FILE_PATH)
    # go up the directory structure until we have the right directory
    while not _expected_directories_are_here(dir_path):
        dir_path = os.path.dirname(dir_path)
        if dir_path == "/":
            raise AssertionError("Failed to find TA dir, recursed till reached root")
    return dir_path


ROBOT_ROOT_PATH = get_TA_dir()


REPO_ROOT_PATH = os.path.dirname(ROBOT_ROOT_PATH)
def get_repo_root_path():
    return REPO_ROOT_PATH

LIBS_PATH = os.path.join(ROBOT_ROOT_PATH, "Libs")
def get_libs_path():
    return LIBS_PATH


RESOURCES_PATH = os.path.join(ROBOT_ROOT_PATH, "resources")
def get_resources_path():
    return RESOURCES_PATH


LOCAL_HTTPS_CERT_PATH = os.path.join(RESOURCES_PATH, "sophos_certs", "local_certs", "https")
def get_local_https_cert_path():
    return LOCAL_HTTPS_CERT_PATH


def addPathToSysPath(p):
    p = os.path.normpath(p)
    if p not in sys.path:
        sys.path.append(p)
