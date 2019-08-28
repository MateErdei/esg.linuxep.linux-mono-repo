
from __future__ import absolute_import,print_function,division,unicode_literals

import os
import unittest
import mock

import sys
import json
import __builtin__

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

def pass_function(*args, **kwargs):
    pass

class FakeMCSConnection(object):
    m_token = None
    m_status = None
    send401 = False

    def __init__(self, config, product_version="unknown",install_dir=".."):
        self.m_send401 = False
        pass

    def set_user_agent(self, agent):
        pass

    def capabilities(self):
        return ""

    def register(self, token, status):
        FakeMCSConnection.m_token = token
        FakeMCSConnection.m_status = status
        return ("newID","newPassword")

    def get_id(self):
        return "FOO"

    def get_password(self):
        return "BAR"

    def query_commands(self, appIds):
        if FakeMCSConnection.send401 and not self.m_send401:
            self.m_send401 = True
            headers = {
                "www-authenticate":'Basic realm="register"',
            }
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException(401,headers,"")
        else:

            raise EscapeException()
        return []

    def send_status_event(self, status):
        raise EscapeException()

    def close(self):
        pass


class TestMCS(unittest.TestCase):
    @mock.patch('os.listdir', return_value=["dummyplugin.json"])
    def createMCS(self, config=None, *mockarg):
        global INSTALL_DIR
        if config is None:
            return mcsrouter.mcs.MCS(INSTALL_DIR)
        return mcsrouter.mcs.MCS(INSTALL_DIR, config)


    def setUp(self):
        ## Monkey patch the MCSConnection
        __builtin__.__dict__['REGISTER_MCS'] = False
        mcsrouter.mcsclient.mcs_connection.MCSConnection = FakeMCSConnection
        FakeMCSConnection.m_token = None
        FakeMCSConnection.m_status = None
        FakeMCSConnection.send401 = False
        PathManager.safeDelete(os.path.join(INSTALL_DIR,"etc","sophosav","mcs.config"))

    def tearDown(self):
        mcsrouter.mcsclient.mcs_connection.MCSConnection = ORIGINAL_MCS_CONNECTION

    # @mock.patch('mcsrouter.utils.plugin_registry.get_app_ids_from_directory', return_value=(set(),{}))
    def testConstruction(self):
        m = self.createMCS()

    def testStartup(self):
        m = self.createMCS()
        m.startup()

    # @mock.patch('mcsrouter.utils.config.Config.get_default', return_value=None)
    def testMCSIDNotSet(self):
        m = self.createMCS(None)
        ret = m.run()
        self.assertEqual(ret,1)

    def testMCSPasswordNotSet(self):
        config = mcsrouter.utils.config.Config()
        config.set("MCSID","FOO")
        self.assertNotEqual(config.get_default("MCSID"),None)
        m = self.createMCS(config)
        ret = m.run()
        self.assertEqual(ret,2)

    @mock.patch('mcsrouter.utils.directory_watcher.DirectoryWatcher.add_watch', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    def testReregistration(self, *mockargs):
        __builtin__.__dict__['REGISTER_MCS'] = True
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

    @mock.patch('mcsrouter.utils.directory_watcher.DirectoryWatcher.add_watch', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('os.listdir', return_value="dummyplugin.json")
    def test401Handling(self, *mockargs):
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
