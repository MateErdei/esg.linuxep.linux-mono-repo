
from __future__ import absolute_import,print_function,division,unicode_literals

import unittest
import sys
import json
import os

import PathManager

import logging
logger = logging.getLogger("TestMCSConnection")

import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception

# Stop the mcs router tests writing to disk
class ConfigWithoutSave(mcsrouter.utils.config.Config):
    def save(self, filename=None):
        pass


def createTestConfig(url="http://localhost/foobar"):
    config = ConfigWithoutSave('testConfig.config')
    config.set("MCSURL",url)
    config.set("MCSID","")
    config.set("MCSPassword","")
    return config

class TestMCSConnection(unittest.TestCase):
    def tearDown(self):
        try:
            os.remove("testConfig.config")
        except Exception:
            pass

    def testUserAgentConstructorEmptyRegToken(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","")
        self.assertEqual(actual,expected)

    def testUserAgentConstructorUnknownRegToken(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","unknown")
        self.assertEqual(actual,expected)

    def testUserAgentConstructorNoneRegToken(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5",None)
        self.assertEqual(actual,expected)

    def testUserAgentConstructorRegToken(self):
        expected = "Sophos MCS Client/5 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","Foobar")
        self.assertEqual(actual,expected)

    def testUserAgentConstructorDiffPlatform(self):
        expected = "Sophos MCS Client/5 MyPlatform sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","Foobar","MyPlatform")
        self.assertEqual(actual,expected)

    def testUserAgentConstructorDiffVersion(self):
        expected = "Sophos MCS Client/99 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("99","Foobar")
        self.assertEqual(actual,expected)

# except ImportError:
#     print >>sys.stderr,sys.path
#     class TestMCSConnection(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()
