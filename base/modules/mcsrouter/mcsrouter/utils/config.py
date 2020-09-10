#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
config Module
"""

import os
import time

from . import path_manager

import logging
LOGGER = logging.getLogger(__name__)

class Config(object):
    """
    Simple key=value configuration file
    """

    def __init__(self, filename=None, parent_config=None, mode=0o640, user_id=0, group_id=0):
        """
        __init__
        """
        self.__m_options = {}
        self.load(filename)
        self.__mode = mode
        self.__group_id = group_id
        self.__user_id = user_id
        self.__m_filename = filename
        self.__m_parent_config = parent_config

    def get_options(self):
        return self.__m_options

    def set(self, key, value):
        """
        set
        """
        self.__m_options[key] = value

    def set_default(self, key, value=None):
        """
        set_default
        """
        if key not in self.__m_options:
            self.__m_options[key] = value

    def remove(self, key):
        """
        remove
        """
        return self.__m_options.pop(key, None) is not None

    def get(self, key):
        """
        get
        """
        if key in self.__m_options:
            return self.__m_options[key]
        if self.__m_parent_config is not None:
            return self.__m_parent_config.get(key)
        return self.__m_options[key]  # KeyError

    def get_default(self, key, default_value=None):
        """
        get_default
        """
        if key in self.__m_options:
            return self.__m_options[key]
        if self.__m_parent_config is not None:
            return self.__m_parent_config.get_default(key, default_value)
        return default_value

    def get_bool(self, key, default_value=True):
        """
        get_bool
        """
        val = self.get_default(key, default_value)
        if val in (True, "1", "true", "True"):
            return True
        if val in (False, "0", "false", "False"):
            return False
        return default_value

    def get_int(self, key, default_value=0):
        """
        get_int
        """
        try:
            return int(self.get_default(key, default_value))
        except ValueError:
            return default_value

    def save(self, filename=None):
        """
        save
        """
        if filename is None:
            filename = self.__m_filename
        assert filename is not None
        old_umask = os.umask(0o777 ^ self.__mode)
        try:
            temp_filename = os.path.join(path_manager.temp_dir(), os.path.basename(filename))
            file_to_write = open(temp_filename, "w")
            for (key, value) in self.__m_options.items():
                file_to_write.write("{}={}\n".format(key, value))
            file_to_write.close()
            os.chown(temp_filename, self.__user_id, self.__group_id)
            os.rename(temp_filename, filename)
        finally:
            os.umask(old_umask)
        self.__m_filename = filename

    def load(self, filename=None):
        """
        load
        """
        if not filename:
            return

        if not os.path.isfile(filename):
            return

        successful_read = False
        for iteration in range(5):
            try:
                with open(filename, 'r') as file_to_read:
                    lines = file_to_read.readlines()
                successful_read = True
            except OSError as error:
                LOGGER.info("Could not read config file {} after {} attempt(s): {}".format(
                    filename,
                    iteration+1,
                    error))
                time.sleep(0.5)

        if not successful_read:
            raise OSError("Failed to read config file {} after 5 attempts".format(filename))

        for line in lines:
            line = line.strip()
            if "=" in line:
                (key, value) = line.split("=", 1)
                self.set(key, value)
