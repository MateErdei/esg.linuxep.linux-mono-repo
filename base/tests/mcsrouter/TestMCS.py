
from __future__ import absolute_import,print_function,division,unicode_literals

import os
import unittest
import sys
import json

import logging
logger = logging.getLogger("TestMCS")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.mcs
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
ORIGINAL_MCS_CONNECTION = mcsrouter.mcsclient.mcs_connection.MCSConnection

import mcsrouter.utils.config

class EscapeException(Exception):
    pass

class FakeMCSConnection(object):
    m_token = None
    m_status = None
    send401 = False

    def __init__(self, config, productVersion="unknown",installDir=".."):
        self.m_send401 = False
        pass

    def setUserAgent(self, agent):
        pass

    def capabilities(self):
        return ""

    def register(self, token, status):
        FakeMCSConnection.m_token = token
        FakeMCSConnection.m_status = status
        return ("newID","newPassword")

    def getId(self):
        return "FOO"

    def getPassword(self):
        return "BAR"

    def queryCommands(self, appIds):
        if FakeMCSConnection.send401 and not self.m_send401:
            self.m_send401 = True
            headers = {
                "www-authenticate":'Basic realm="register"',
            }
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException(401,headers,"")
        else:

            raise EscapeException()
        return []

    def sendStatusEvent(self, status):
        raise EscapeException()

    def close(self):
        pass

class TestMCS(unittest.TestCase):
    def createMCS(self,config=None):
        global INSTALL_DIR
        if config is None:
            config = mcsrouter.utils.config.Config()
        return mcsrouter.mcs.MCS(config,INSTALL_DIR)

    def setUp(self):
        ## Monkey patch the MCSConnection
        mcsrouter.mcsclient.mcs_connection.MCSConnection = FakeMCSConnection
        FakeMCSConnection.m_token = None
        FakeMCSConnection.m_status = None
        FakeMCSConnection.send401 = False
        PathManager.safeDelete(os.path.join(INSTALL_DIR,"etc","sophosav","mcs.config"))

    def tearDown(self):
        mcsrouter.mcsclient.mcs_connection.MCSConnection = ORIGINAL_MCS_CONNECTION

    def testConstruction(self):
        m = self.createMCS()

    def testStartup(self):
        m = self.createMCS()
        m.startup()

    def testMCSIDNotSet(self):
        m = self.createMCS()
        ret = m.run()
        self.assertEqual(ret,1)

    def testMCSPasswordNotSet(self):
        config = mcsrouter.utils.config.Config()
        config.set("MCSID","FOO")
        self.assertNotEqual(config.getDefault("MCSID"),None)
        m = self.createMCS(config)
        ret = m.run()
        self.assertEqual(ret,2)

    def testReregistration(self):
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","reregister")
        config.set("MCSPassword","")
        m = self.createMCS(config)
        try:
            ret = m.run()
            self.fail("Shouldn't run() to completion")
        except EscapeException:
            pass
        self.assertEqual(FakeMCSConnection.m_token,"MCSTOKEN") ## Verifies register called

    def test401Handling(self):
        FakeMCSConnection.send401 = True
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","0")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","0")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY","0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY","0")
        m = self.createMCS(config)
        try:
            ret = m.run()
            self.fail("Shouldn't run() to completion")
        except EscapeException:
            pass
        self.assertEqual(FakeMCSConnection.m_token,"MCSTOKEN") ## Verifies register called

class TestCommandCheckInterval(unittest.TestCase):
    def testCreation(self):
        config = mcsrouter.utils.config.Config()
        c = mcsrouter.mcs.CommandCheckInterval(config)

    def testDefaultPollingInterval(self):
        config = mcsrouter.utils.config.Config()
        c = mcsrouter.mcs.CommandCheckInterval(config)
        self.assertEqual(c.get(),mcsrouter.mcs.CommandCheckInterval.DEFAULT_MIN_POLLING_INTERVAL)

    def testPreCreationIntervalConfig(self):
        config = mcsrouter.utils.config.Config()
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","53")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","53")
        c = mcsrouter.mcs.CommandCheckInterval(config)
        self.assertEqual(c.get(),53)

    def testPostCreationIntervalConfig(self):
        config = mcsrouter.utils.config.Config()
        c = mcsrouter.mcs.CommandCheckInterval(config)
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","53")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","53")
        c.set(20)
        self.assertEqual(c.get(),53)

# except ImportError:
#     logger.error("Bad sys.path: %s",str(sys.path))
#     class TestMCS(unittest.TestCase):
#         pass

if __name__ == '__main__':
    import logging
    unittest.main()
