#!/usr/bin/env python

import os

class Config(object):
    """
    Simple key=value configuration file
    """
    def __init__(self,filename=None,parentConfig=None):
        self.__m_options = {}
        self.load(filename)
        self.__m_filename = filename
        self.__m_parentConfig = parentConfig

    def set(self, key, value):
        self.__m_options[key] = value

    def setDefault(self, key, value=None):
        if not self.__m_options.has_key(key):
            self.__m_options[key] = value

    def remove(self, key):
        return self.__m_options.pop(key,None) is not None

    def get(self, key):
        if self.__m_options.has_key(key):
            return self.__m_options[key]
        if self.__m_parentConfig is not None:
            return self.__m_parentConfig.get(key)
        return self.__m_options[key] ## KeyError

    def getDefault(self, key, defaultValue=None):
        if self.__m_options.has_key(key):
            return self.__m_options[key]
        if self.__m_parentConfig is not None:
            return self.__m_parentConfig.getDefault(key,defaultValue)
        return defaultValue

    def getBool(self, key, defaultValue=True):
        val = self.getDefault(key, defaultValue)
        if val in (True,"1","true","True"):
            return True
        if val in (False,"0","false","False"):
            return False
        return defaultValue

    def getInt(self, key, defaultValue=0):
        try:
            return int(self.getDefault(key,defaultValue))
        except ValueError:
            return defaultValue

    def save(self, filename=None, mode=0o600):
        if filename is None:
            filename = self.__m_filename
        assert filename is not None
        old_umask = os.umask(0o777 ^ mode)
        try:
            f = open(filename,"w")
            os.chmod(filename,mode)
            for (key,value) in self.__m_options.iteritems():
                f.write("%s=%s\n"%(key,value))
            f.close()
        finally:
            os.umask(old_umask)
        self.__m_filename = filename

    def load(self, filename=None):
        if filename is None:
            return

        if not os.path.isfile(filename):
            return

        f = open(filename)
        lines = f.readlines()
        f.close()

        for line in lines:
            line = line.strip()
            if "=" in line:
                (key,value) = line.split("=",1)
                self.set(key,value)

