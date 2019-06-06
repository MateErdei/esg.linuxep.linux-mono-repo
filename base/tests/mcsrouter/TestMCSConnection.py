
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

def createTestConfig(url="http://localhost/foobar"):
    config = mcsrouter.utils.config.Config('testConfig.config')
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

    def testUserAgent(self):
        config = createTestConfig("http://allegro.eng.sophos:10443/unit/headers")
        try:
            conn = mcsrouter.mcsclient.mcs_connection.MCSConnection(config)
            results = conn.capabilities()
        except mcsrouter.mcsclient.mcs_exception.MCSConnectionFailedException as e:
            ## Not able to make connection to allegro
            self.skipTest("Not Able to connect to allegro")
            return

        #~ print(results)
        headers = {}
        for line in results.splitlines():
            if "=" not in line:
                continue
            k,v = line.split("=",1)
            headers[k] = v
        #~ print(json.dumps(headers,indent=2,sort_keys=True))
        userAgent = headers['HTTP_USER_AGENT']
        logger.debug("UserAgent = %s",userAgent)
        self.assertTrue(userAgent.startswith("Sophos MCS Client"))

    def testCookieSetting(self):
        config = createTestConfig("http://allegro.eng.sophos:10443/unit/cookie")
        try:
            conn = mcsrouter.mcsclient.mcs_connection.MCSConnection(
                config)
            results = conn.send_message("?ARGS=1","1")
            logger.debug("1 %s",str(results))
            self.assertTrue("Cookies 0" in results)
            results = conn.send_message("?ARGS=2","2")
            logger.debug("2 %s",str(results))
            self.assertTrue("Cookies 1" in results)
        except mcsrouter.mcsclient.mcs_exception.MCSConnectionFailedException as e:
            ## Not able to make connection to allegro
            self.skipTest("Not Able to connect to allegro")
            return

# except ImportError:
#     print >>sys.stderr,sys.path
#     class TestMCSConnection(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()
