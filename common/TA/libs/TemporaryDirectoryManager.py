#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import re
import os
import shutil
from robot.api import logger

temporary_directory_rootdir = "/tmp"  # All temporary directories are made under /tmp
regex_pattern_for_valid_temporary_directories = "^[a-zA-Z0-9]*$"

class TemporaryDirectory():

    def __init__(self, name):
        if not re.match(regex_pattern_for_valid_temporary_directories, name):
            logger.error("ERROR: Directory name must consist of only lower and upper case characters" 
                         "(i.e. as regex: \"{}\")".format(regex_pattern_for_valid_temporary_directories))
            logger.info("Temporary directories are all placed into the \"{}\" directory. Do not give a path."
                        .format(temporary_directory_rootdir))
            raise AssertionError
        self.m_name = name
        self.m_path = "{}/{}".format(temporary_directory_rootdir, name)
        self._safe_mkdir()

    def __del__(self):
        if os.path.isdir(self.m_path):
            logger.warn("WARNING: \"{}\" was not cleaned up, attempting to remove now".format(self.m_path))
            self._safe_rmtree()

    def _safe_rmtree(self):
        if not os.path.isdir(self.m_path):
            raise AssertionError("Temporary directory:\"{}\" has already been removed, this violates the contract"
                                 "with the temporary directory manager")
        try:
            shutil.rmtree(self.m_path)
            logger.info("\"{}\" has been removed".format(self.m_path))
        except Exception as e:
            failure_string = "Failed to remove directory at \"{}\" for reason: {}".format(self.m_path, e)
            logger.error(failure_string)
            raise Exception(failure_string)
        assert not os.path.isdir(self.m_path)

    def _safe_mkdir(self):
        try:
            if os.path.isdir(self.m_path):
                raise AssertionError("\"{}\" already exists".format(self.m_path))
            else:
                os.mkdir(self.m_path)
        except OSError as e:
            failure_string = "Failed to create directory at \"{}\" for reason: {}".format(self.m_path, e)
            logger.error(failure_string)
            raise OSError(failure_string)
        assert os.path.isdir(self.m_path)

    def remove(self):
        if not os.path.isdir(self.m_path):
            raise Exception("\"{}\" does not exist".format(self.m_path))
        else:
            self._safe_rmtree()

"""
Expected Usage (in robot):

# in your test:

    ${TEMPORARY_DIRECTORIES} =  Add Temporary Directory  DirectoryName 
    
    # "Add Temporary Directory" makes a directory with the name you pass to it in the /tmp/ directory.
    # Do not try and pass it a path, it will be rejected
    # It will return a full path to the directory which it creates, "TEMPORARY_DIRECTORIES" in the above example
    # this should be used as a variable for the rest of the test to refer to the directory
    
# In your test teardown:

    Cleanup Temporary Folders
    
    # Can also be done in a suite teardown. Additionally, there is a call in the global teardown which
    # will enforce that at least one cleanup is called at the end of the run
    # Failure to adhere to this contract will throw warnings/errors

"""
class TemporaryDirectoryManager():

    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    ROBOT_LISTENER_API_VERSION = 2

    def __init__(self):
        self.ROBOT_LIBRARY_LISTENER = self
        self.m_directories = {}

    def __del__(self):
        if self.m_directories:
            self.cleanup_temporary_folders()
            logger.warn("\"cleanup_temporary_folders\" was not called excplicitly to clean up temporary directories")

    def _close(self):
        self.__del__()

    def add_temporary_directory(self, dirname):
        if not re.match(regex_pattern_for_valid_temporary_directories, dirname):
            logger.error("ERROR: Directory name must consist of only lower and upper case characters"
                         "(i.e. as regex: \"{}\")".format(regex_pattern_for_valid_temporary_directories))
            logger.info("Temporary directories are all placed into the \"{}\" directory. Do not give a path."
                        .format(temporary_directory_rootdir))
            raise AssertionError
        if dirname in self.m_directories:
            raise AssertionError("\"{}\" is already a temporary directory".format(dirname))
        self.m_directories[dirname] = TemporaryDirectory(dirname)

        return "{}/{}".format(temporary_directory_rootdir, dirname)

    def remove_temporary_directory(self, dirname):
        if dirname not in self.m_directories:
            raise AssertionError("\"{}\" is not a temporary directory".format(dirname))
        else:
            self.m_directories[dirname].remove()
            self.m_directories.pop(dirname)

    def cleanup_temporary_folders(self):
        for directory_key in list(self.m_directories.keys()):
            logger.info("removing: \"{}\"".format(directory_key))
            self.remove_temporary_directory(directory_key)
