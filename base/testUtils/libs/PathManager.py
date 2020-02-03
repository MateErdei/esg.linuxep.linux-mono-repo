#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019-2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import sys
from robot.api import logger

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
    dir_path = os.path.dirname(THIS_FILE_PATH)
    #loops until "sspl-plugin-mdr-component" is in directory pointed to by dir_path
    while not are_basenames_in_directory(dir_path, [component_dirname]):
        dir_path = os.path.dirname(dir_path)
        print(dir_path)
        if dir_path ==  "/":
            logger.warning("Failed to find {} dir, recursed till reached root".format(component_dirname))
    return os.path.join(dir_path, component_dirname)
    

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

ROOT_PATH = get_testUtils_dir()

SUPPORTFILEPATH = os.path.join(ROOT_PATH, "SupportFiles")
def get_support_file_path():
    return SUPPORTFILEPATH

LIBS_PATH = os.path.join(ROOT_PATH, "libs")
def get_libs_path():
    return LIBS_PATH

ROBOT_TESTS_PATH = os.path.join(ROOT_PATH, "tests")


def addPathToSysPath(p):
    p = os.path.normpath(p)
    if p not in sys.path:
        sys.path.append(p)
