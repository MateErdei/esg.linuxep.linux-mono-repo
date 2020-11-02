


import os
import unittest
import mock

import sys
import time
import json
import builtins

import logging
logger = logging.getLogger("TestMCS")

BUILD_DIR=os.environ.get("ABS_BUILDDIR",".")
INSTALL_DIR=os.path.join(BUILD_DIR,"install")

import PathManager

import mcsrouter.mcs
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_commands as mcs_commands
import mcsrouter.adapters.generic_adapter as generic_adapter
import mcsrouter.mcs_push_client
ORIGINAL_MCS_CONNECTION = mcsrouter.mcsclient.mcs_connection.MCSConnection
FLAG_FILE = "/tmp/flags-mcs.json"
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

    def get_flags(self):
        return "{flag : true}"

    def close(self):
        pass

    def set_jwt_token_settings(self):
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
        builtins.__dict__['REGISTER_MCS'] = False
        mcsrouter.mcsclient.mcs_connection.MCSConnection = FakeMCSConnection
        FakeMCSConnection.m_token = None
        FakeMCSConnection.m_status = None
        FakeMCSConnection.send401 = False
        PathManager.safeDelete(os.path.join(INSTALL_DIR,"etc","sophosav","mcs.config"))

    def tearDown(self):
        mcsrouter.mcsclient.mcs_connection.MCSConnection = ORIGINAL_MCS_CONNECTION
        if os.path.exists(FLAG_FILE):
            os.remove(FLAG_FILE)

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
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testReregistration(self, *mockargs):
        builtins.__dict__['REGISTER_MCS'] = True
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
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
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

    @mock.patch('mcsrouter.utils.path_manager.mcs_flags_file', return_value=FLAG_FILE)
    @mock.patch('mcsrouter.utils.directory_watcher.DirectoryWatcher.add_watch', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('os.listdir', return_value="dummyplugin.json")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testgetFlags(self, *mockargs):
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
            ret = m.startup()
            ret = m.get_flags(time.time()-1)

        except EscapeException:
            pass
        self.assertEqual(True, os.path.isfile(FLAG_FILE))
        with open(FLAG_FILE, 'r') as outfile:
            content = outfile.read()
        self.assertEqual(content, "{flag : true}")

    @mock.patch('mcsrouter.utils.path_manager.mcs_flags_file', return_value=FLAG_FILE)
    @mock.patch('mcsrouter.utils.directory_watcher.DirectoryWatcher.add_watch', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('os.listdir', return_value="dummyplugin.json")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testgetFlagsOnlyUpdatesWhenTheFlagsPollingIntervalHasBeenReached(self, *mockargs):
        FakeMCSConnection.send401 = True
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","0")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","300")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY","0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY","0")
        m = self.createMCS(config)

        try:
            ret = m.startup()
            with open(FLAG_FILE, 'w') as outfile:
                outfile.write("stuff")
            # When this runs the last time check will be under the COMMAND_CHECK_INTERVAL_MAXIMUM
            # so the file will not be updated with the flags
            ret = m.get_flags(time.time())

        except EscapeException:
            pass
        self.assertEqual(True, os.path.isfile(FLAG_FILE))
        with open(FLAG_FILE, 'r') as outfile:
            content = outfile.read()
        self.assertEqual(content, "stuff")

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_user_agent")
    def test_user_agent_set_during_startup(self, *mockargs):
        version_file_content = """
        PRODUCT_NAME = Sophos Server Protection Linux - Base Component
        PRODUCT_VERSION = 1.1.3.703
        BUILD_DATE = 2020-08-14
        """
        m = self.createMCS()
        with mock.patch('os.path.isfile', return_value=True):
            with mock.patch("builtins.open", mock.mock_open(read_data=version_file_content)):
                m.startup()
                self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.set_user_agent.call_count, 1)
                mcsrouter.mcsclient.mcs_connection.MCSConnection.set_user_agent.assert_called_with('Sophos MCS Client/1.1.3.703 Linux sessions regToken')

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


    def test_set_interval_when_use_fallback_polling_interval_set_to_true(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)

        #is updated to the push fall back poll
        config.set("PUSH_SERVER_CHECK_INTERVAL", "4400")
        command_check_interval.set_use_fallback_polling_interval(True)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 4400)

    def test_interval_increment_when_use_fallback_polling_interval_set_to_true(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)


        config.set("PUSH_SERVER_CHECK_INTERVAL", "4400")
        command_check_interval.set_use_fallback_polling_interval(True)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 4400)
        command_check_interval.increment()
        # expect the interval not to change.
        self.assertEqual(command_check_interval.get(), 4400)

    def test_set_interval_when_use_fallback_polling_interval_set_to_False(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)

        #is updated to the push fall back poll
        config.set("PUSH_SERVER_CHECK_INTERVAL", "4400")
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","53")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","53")
        command_check_interval.set_use_fallback_polling_interval(False)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 53)

    def test_interval_increment_when_use_fallback_polling_interval_set_to_False(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)


        config.set("PUSH_SERVER_CHECK_INTERVAL", "4400")
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","53")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","83")
        command_check_interval.set_use_fallback_polling_interval(False)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 53)
        command_check_interval.increment()
        # expect the interval to be incremented.
        self.assertEqual(command_check_interval.get(), (53 + 20))

    def test_interval_will_revert_to_mcs_default_polling_when_use_fallback_polling_is_set_to_False(self):
        config = mcsrouter.utils.config.Config()
        config.set("PUSH_SERVER_CHECK_INTERVAL", "5500")
        config.set("COMMAND_CHECK_INTERVAL_MINIMUM","53")
        config.set("COMMAND_CHECK_INTERVAL_MAXIMUM","53")

        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set_use_fallback_polling_interval(True)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 5500)
        command_check_interval.set_use_fallback_polling_interval(False)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 53)



if __name__ == '__main__':
    import logging
    unittest.main()
