#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, division, unicode_literals

import os
import sys
import errno

import logging
logger = logging.getLogger(__name__)


def appendPath(d):
    if d not in sys.path:
        sys.path.append(d)


def get_modules_dir():
    UNIT_TEST_DIR = os.path.dirname(__file__)
    TEST_DIR = os.path.dirname(UNIT_TEST_DIR)
    SOURCE_DIR = os.path.dirname(TEST_DIR)
    MODULES_DIR = os.path.join(SOURCE_DIR, "modules")
    return MODULES_DIR

def get_mcs_router_dir():
    return os.path.join(get_modules_dir(), "mcsrouter")

def safeRmtree(path):
    try:
        shutil.rmtree(path,ignore_errors=True)
    except EnvironmentError,e:
        if e.errno == errno.ENOENT:
            return
        else:
            raise

def safeDelete(path):
    try:
        os.unlink(path)
    except EnvironmentError, e:
        if e.errno == errno.ENOENT:
            return
        else:
            raise

def safeMkdir(path, perm=0700):
    try:
        os.makedirs(path)
        os.chmod(path,perm)
    except EnvironmentError,e:
        if e.errno == errno.EEXIST:
            return
        else:
            raise

def writeFile(path,content,perm=0600):
    open(path,"w").write(content)
    os.chmod(path,perm)

appendPath(get_mcs_router_dir())

class FakePolicyCommand(object):
    def __init__(self, policy):
        self.m_policy = policy
        self.m_complete = False

    def get_policy(self):
        return self.m_policy

    def complete(self):
        self.m_complete = True

    def __str__(self):
        return "FakePolicyCommand"


class FakeConfigManager(object):
    def __init__(self):
        self.m_config = {}
        self.m_used = False

    def openAndLockConnection(self):
        pass

    def unlockAndCloseConnection(self):
        pass

    def set(self, savPath, savValue, lock=True, flags=0):
        assert isinstance(savValue,basestring) or isinstance(savValue,list)
        self.m_config[savPath] = savValue
        self.m_used = True
        logger.info("Set %s to %s",savPath,str(savValue))


    def delete(self, savPath):
        self.m_config.pop(savPath,"")
        self.m_used = True
        logger.info("Deleting %s",savPath)

    def sendEvent(self, event):
        self.m_used = True
        logger.info("Sending event")


    def queryList(self, savPath):
        self.m_used = True
        logger.info("queryList")
        return []

    def add(self, savPath, savValue):
        self.m_used = True
        logger.info("Add %s to %s",str(savValue),savPath)
        self.m_config.setdefault(savPath,[]).append(savValue)


def setFakeConfigManager():
    configManager = FakeConfigManager()
    mcsrouter.adapters.ConfigManager.gGlobalConfigManager = configManager
    return configManager

def resetConfigManager():
    mcsrouter.adapters.ConfigManager.gGlobalConfigManager = None
