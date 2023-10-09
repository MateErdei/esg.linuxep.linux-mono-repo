#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright 2019-2023 Sophos Limited. All rights reserved.

import os
import sys
try:
    from robot.api import logger
except:
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


def find_local_component_dir_path(component_dirname):
    attempts = []
    for dir_path in (os.path.dirname(THIS_FILE_PATH), os.path.dirname(__file__), os.getcwd()):
        # loops until "component_dirname" is in directory pointed to by dir_path
        while not are_basenames_in_directory(dir_path, [component_dirname]):
            attempts.append(dir_path)
            dir_path = os.path.dirname(dir_path)
            print(dir_path)
            if dir_path == "/":
                break
        if dir_path != "/":
            return os.path.join(dir_path, component_dirname)

    logger.info("Failed to find {} dir, recursed till reached root from {}: tried {}".format(
        component_dirname,
        os.path.dirname(THIS_FILE_PATH),
        attempts))
    return None
    

def find_local_base_dir_path():
    return find_local_component_dir_path("everest-base")


def find_local_mtr_dir_path():
    return find_local_component_dir_path("sspl-plugin-mdr-component")
    

def libs_supportfiles_and_tests_are_here(dir_path):
    return are_basenames_in_directory(dir_path, ["libs", "SupportFiles", "tests", "testUtilsMarker"])


def get_testUtils_dir():
    dir_path = os.path.dirname(THIS_FILE_PATH)
    # go up the directory structure until we have the right directory
    while not libs_supportfiles_and_tests_are_here(dir_path):
        dir_path = os.path.dirname(dir_path)
        if dir_path ==  "/":
            raise AssertionError("Failed to find testUtils dir, recursed till reached root")
    return dir_path

ROBOT_ROOT_PATH = get_testUtils_dir()

REPO_ROOT_PATH = os.path.dirname(ROBOT_ROOT_PATH)
def get_repo_root_path():
    return REPO_ROOT_PATH

SUPPORTFILEPATH = os.path.join(ROBOT_ROOT_PATH, "SupportFiles")
def get_support_file_path():
    return SUPPORTFILEPATH

LIBS_PATH = os.path.join(ROBOT_ROOT_PATH, "libs")
def get_libs_path():
    return LIBS_PATH

ROBOT_TESTS_PATH = os.path.join(ROBOT_ROOT_PATH, "tests")
def get_robot_tests_path():
    return ROBOT_TESTS_PATH

def addPathToSysPath(p):
    p = os.path.normpath(p)
    if p not in sys.path:
        sys.path.append(p)


SYSTEM_PRODUCT_TEST_INPUTS = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")

SOPHOS_INSTALL = os.path.join("/", "opt", "sophos-spl")
