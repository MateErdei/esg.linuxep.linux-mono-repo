#!/usr/bin/env python
"""
config Module
"""

import os
import mcsrouter.utils.path_manager


class Config(object):
    """
    Simple key=value configuration file
    """

    def __init__(self, filename=None, parent_config=None, mode=0o600, user_id=0, group_id=0):
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
                file_to_write.write("%s=%s\n" % (key, value))
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
        if filename is None:
            return

        if not os.path.isfile(filename):
            return

        file_to_read = open(filename)
        lines = file_to_read.readlines()
        file_to_read.close()

        for line in lines:
            line = line.strip()
            if "=" in line:
                (key, value) = line.split("=", 1)
                self.set(key, value)
