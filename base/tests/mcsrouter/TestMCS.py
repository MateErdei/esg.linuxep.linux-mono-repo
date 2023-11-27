import os
import unittest
from unittest import mock

import sys
import time
import json
import builtins
import xml.dom.minidom

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
from mcsrouter.utils import signal_handler


class EscapeException(Exception):
    pass

def pass_function(*args, **kwargs):
    pass

class FakeMCSConnection(object):
    m_token = None
    m_status = None
    m_migration_event = None
    m_migration_response = None
    m_device_id = None
    m_tenant_id = None
    send401 = False
    send404 = False
    m_cookies = {}

    def __init__(self, config, product_version="unknown",install_dir="..",migrate_mode=False):
        self.m_send401 = False
        self.m_send404 = False
        pass

    def set_user_agent(self, agent):
        pass

    def clear_cookies(self):
        self.m_cookies.clear()

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
                "www-authenticate": 'Basic realm="register"',
            }
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException(401, headers, "")
        elif FakeMCSConnection.send404 and not self.m_send404:
            self.m_send404 = True
            headers = {}
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpException(404, headers, "")
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

    def set_migrate_mode(self, migrate_mode: bool):
        pass

    def send_migration_event(self, event):
        self.m_migration_event = event

    def send_migration_request(self, server_url, jwt_token, request_body, old_device_id, old_tenant_id):
        if FakeMCSConnection.send401 and not self.m_send401:
            self.m_send401 = True
            headers = { }
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException(401,headers,"")
        elif FakeMCSConnection.send404 and not self.m_send404:
            self.m_send404 = True
            headers = { }
            raise mcsrouter.mcsclient.mcs_connection.MCSHttpUnauthorizedException(404,headers,"")
        return self.m_migration_response


class TestMCS(unittest.TestCase):
    @mock.patch('os.listdir', return_value=["dummyplugin.json"])
    def createMCS(self, config=None, *mockarg):
        global INSTALL_DIR
        signal_handler.setup_signal_handler()
        if config is None:
            return mcsrouter.mcs.MCS(INSTALL_DIR)
        return mcsrouter.mcs.MCS(INSTALL_DIR, config)


    def setUp(self):
        ## Monkey patch the MCSConnection
        builtins.__dict__['REGISTER_MCS'] = False
        mcsrouter.mcsclient.mcs_connection.MCSConnection = FakeMCSConnection
        FakeMCSConnection.m_token = None
        FakeMCSConnection.m_status = None
        FakeMCSConnection.m_migration_event = None
        FakeMCSConnection.m_migration_response = None
        FakeMCSConnection.send401 = False
        FakeMCSConnection.send404 = False

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
        config.set("MCSPassword","password")
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
        config.set("commandPollingDelay","0")
        config.set("flagsPollingInterval","0")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY","0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY","0")
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
    def testHTTPErrorClearsCookies(self, *mockargs):
        FakeMCSConnection.send404 = True
        FakeMCSConnection.m_cookies = {"should not": "exist after HTTP error"}
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken", "MCSTOKEN")
        config.set("MCSID", "FOO")
        config.set("MCSPassword", "BAR")
        config.set("commandPollingDelay", "0")
        config.set("flagsPollingInterval", "0")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY", "0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY", "0")
        m = self.createMCS(config)
        try:
            ret = m.run()
            self.fail("Shouldn't run() to completion")
        except EscapeException:
            pass

        # Verify cookies cleared on HTTP error
        self.assertEqual(FakeMCSConnection.m_cookies, {})

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
        config.set("commandPollingDelay","0")
        config.set("flagsPollingInterval","0")
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
        config.set("commandPollingDelay","0")
        config.set("flagsPollingInterval","300")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY","0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY","0")
        m = self.createMCS(config)

        try:
            ret = m.startup()
            with open(FLAG_FILE, 'w') as outfile:
                outfile.write("stuff")
            # When this runs the last time check will be under the flagsPollingInterval
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
        PRODUCT_NAME = SPL-Base-Component
        PRODUCT_VERSION = 1.1.3.703
        BUILD_DATE = 2020-08-14
        """
        m = self.createMCS()
        with mock.patch('os.path.isfile', return_value=True):
            with mock.patch("builtins.open", mock.mock_open(read_data=version_file_content)):
                m.startup()
                self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.set_user_agent.call_count, 1)
                mcsrouter.mcsclient.mcs_connection.MCSConnection.set_user_agent.assert_called_with('Sophos MCS Client/1.1.3.703 Linux sessions regToken')

    def test_process_deployment_response_body_returns_token_when_product_is_supported(self):
        m = self.createMCS()

        GOOD_BODY = """{"dciFileName":"db55fcf8898da2b3f3c06f26e9246cbb",\
        "registrationToken":"9b53995c2efccba4a25a6118f5f0434eb28db001e131c1211a7c15e052faa38f",\
        "products":[{"product":"MDR","supported":true,"reasons":[]},{"product":"ANTIVIRUS","supported":true,"reasons":[]}]}\
        """

        self.assertEqual(m.process_deployment_response_body(GOOD_BODY), "9b53995c2efccba4a25a6118f5f0434eb28db001e131c1211a7c15e052faa38f")

    def test_process_deployment_response_body_returns_token_when_requested_product_is_unsupported(self):
        m = self.createMCS()

        GOOD_BODY_UNSUPPORTED = """{"dciFileName":"db55fcf8898da2b3f3c06f26e9246cbb",
        "products":[{"product":"DEVICE_ENCRYPTION","supported":false,"reasons":["UNSUPPORTED_PLATFORM","UNLICENSED"]}],\
        "registrationToken":"5128db71f1f32f095b9c133cac308f78c50516b07d1af5b0a637a891b54ff706"}\
        """

        self.assertEqual(m.process_deployment_response_body(GOOD_BODY_UNSUPPORTED), "5128db71f1f32f095b9c133cac308f78c50516b07d1af5b0a637a891b54ff706")

    def test_process_deployment_response_body_throws_when_given_invalid_json(self):
        m = self.createMCS()

        self.assertRaises(mcsrouter.mcs.DeploymentApiException, m.process_deployment_response_body, "garbage")

    def test_process_deployment_response_body_throws_given_garbage_input(self):
        m = self.createMCS()

        BODY_WITHOUT_TOKEN = """{"dciFileName":"db55fcf8898da2b3f3c06f26e9246cbb",
        "products":[{"product":"DEVICE_ENCRYPTION","supported":false,"reasons":["UNSUPPORTED_PLATFORM","UNLICENSED"]}]}\
        """
        # When No Token
        self.assertRaises(mcsrouter.mcs.DeploymentApiException, m.process_deployment_response_body, BODY_WITHOUT_TOKEN)

    @mock.patch('mcsrouter.utils.path_manager.mcs_flags_file', return_value=FLAG_FILE)
    @mock.patch('mcsrouter.utils.directory_watcher.DirectoryWatcher.add_watch', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="this is not xml")
    def testMalformedMigrationActionIsIgnored(self, *mockargs):

        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        config.set("commandPollingDelay","0")
        config.set("flagsPollingInterval","0")
        config.set("COMMAND_CHECK_BASE_RETRY_DELAY","0")
        config.set("COMMAND_CHECK_SEMI_PERMANENT_RETRY_DELAY","0")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        # The XML parsing error is expected to be caught safely within attempt_migration
        self.assertFalse(m.attempt_migration(None, None, None, None))

    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save_non_atomic', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>server name</server><token>jwt</token></action>""")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode")
    @mock.patch('mcsrouter.adapters.agent_adapter.AgentAdapter.get_policy_status', return_value="""<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testMigrationAction(self, *mockargs):
        FakeMCSConnection.m_migration_response = '{"endpoint_id":"newEndpoint","device_id":"newDevice","tenant_id":"newTenant","password":"newPassword"}'
        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        comms = FakeMCSConnection(config)
        root_config = config

        self.assertTrue(m.attempt_migration(comms, config, root_config, None))
        self.assertNotIn('MCSToken', root_config.get_options())
        self.assertEqual(root_config.get('MCSURL'), 'server name')
        self.assertEqual(FakeMCSConnection.set_migrate_mode.call_count, 2)
        self.assertIsNotNone(comms.m_migration_event)
        doc = xml.dom.minidom.parseString(comms.m_migration_event)
        event = doc.getElementsByTagName('event')[0]
        token = event.getElementsByTagName('token')[0]
        self.assertEqual(event.getAttribute('type'), 'sophos.mgt.mcs.migrate.succeeded')
        self.assertEqual(token.firstChild.data, 'jwt')

    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save_non_atomic', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server></server><token>jwt</token></action>""")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode")
    @mock.patch('mcsrouter.adapters.agent_adapter.AgentAdapter.get_policy_status', return_value="""<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testMigrationActionWithEmptyServer(self, *mockargs):
        FakeMCSConnection.send404 = True
        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        comms = FakeMCSConnection(config)
        root_config = config

        self.assertFalse(m.attempt_migration(comms, config, root_config, None))
        self.assertEqual(FakeMCSConnection.set_migrate_mode.call_count, 0)
        self.assertEqual(root_config.get('MCSToken'), 'MCSTOKEN')

    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save_non_atomic', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>server name</server><token>jwt</token></action>""")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode")
    @mock.patch('mcsrouter.adapters.agent_adapter.AgentAdapter.get_policy_status', return_value="""<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testMigrationActionWithInvalidToken(self, *mockargs):
        FakeMCSConnection.send401 = True
        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        comms = FakeMCSConnection(config)
        root_config = config

        self.assertFalse(m.attempt_migration(comms, config, root_config, None))
        self.assertEqual(FakeMCSConnection.set_migrate_mode.call_count, 0)
        self.assertEqual(root_config.get('MCSToken'), 'MCSTOKEN')

    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save_non_atomic', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>server name</server><token>jwt</token></action>""")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode")
    @mock.patch('mcsrouter.adapters.agent_adapter.AgentAdapter.get_policy_status', return_value="""<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testMigrationActionWithMismatchedResponseTypes(self, *mockargs):
        FakeMCSConnection.m_migration_response = '{"endpoint_id":0,"device_id":1,"tenant_id":true,"password":false}'
        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        comms = FakeMCSConnection(config)
        root_config = config

        self.assertTrue(m.attempt_migration(comms, config, root_config, None))
        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode.call_count, 2)
        self.assertNotIn('MCSToken', root_config.get_options())
        self.assertEqual(root_config.get('MCSURL'), 'server name')
        self.assertEqual(FakeMCSConnection.set_migrate_mode.call_count, 2)
        self.assertIsNotNone(comms.m_migration_event)
        doc = xml.dom.minidom.parseString(comms.m_migration_event)
        event = doc.getElementsByTagName('event')[0]
        token = event.getElementsByTagName('token')[0]
        self.assertEqual(event.getAttribute('type'), 'sophos.mgt.mcs.migrate.succeeded')
        self.assertEqual(token.firstChild.data, 'jwt')

    @mock.patch('mcsrouter.utils.config.Config.save', side_effect=pass_function)
    @mock.patch('mcsrouter.utils.config.Config.save_non_atomic', side_effect=pass_function)
    @mock.patch('os.listdir', return_value=[])
    @mock.patch('mcsrouter.adapters.app_proxy_adapter.AppProxyAdapter.get_migrate_action', return_value="""<?xml version='1.0'?><action type="sophos.mgt.mcs.migrate"><server>server name</server><token>jwt</token></action>""")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.set_migrate_mode")
    @mock.patch('mcsrouter.adapters.agent_adapter.AgentAdapter.get_policy_status', return_value="""<policy-status latest="1970-01-01T00:00:00.0Z"><policy app="ALC" latest="2019-09-05T10:02:14.499865Z" /></policy-status>""")
    @mock.patch('mcsrouter.adapters.agent_adapter.ComputerCommonStatus.get_mac_addresses', return_value=["12:34:56:78:12:34"])
    def testMigrationActionWithMalformedResponse(self, *mockargs):
        FakeMCSConnection.m_migration_response = 'this is not json'
        # General setup for MCS tests that need a comms object
        config = mcsrouter.utils.config.Config()
        config.set("MCSToken","MCSTOKEN")
        config.set("MCSID","FOO")
        config.set("MCSPassword","BAR")
        m = self.createMCS(config)
        try:
            ret = m.startup()
        except EscapeException:
            pass

        comms = FakeMCSConnection(config)
        root_config = config

        self.assertFalse(m.attempt_migration(comms, config, root_config, None))
        self.assertEqual(FakeMCSConnection.set_migrate_mode.call_count, 0)
        self.assertEqual(root_config.get('MCSToken'), 'MCSTOKEN')

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
        config.set("commandPollingDelay","53")
        config.set("flagsPollingInterval","53")
        c = mcsrouter.mcs.CommandCheckInterval(config)
        self.assertEqual(c.get(),53)

    def testPostCreationIntervalConfig(self):
        config = mcsrouter.utils.config.Config()
        c = mcsrouter.mcs.CommandCheckInterval(config)
        config.set("commandPollingDelay","53")
        config.set("flagsPollingInterval","53")
        c.set(20)
        self.assertEqual(c.get(),53)


    def test_set_interval_when_use_fallback_polling_interval_set_to_true(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)

        #is updated to the push fall back poll
        config.set("pushFallbackPollInterval", "4400")
        command_check_interval.set_use_fallback_polling_interval(True)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 4400)

    def test_interval_increment_when_use_fallback_polling_interval_set_to_true(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)


        config.set("pushFallbackPollInterval", "4400")
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
        config.set("pushFallbackPollInterval", "4400")
        config.set("commandPollingDelay","53")
        config.set("flagsPollingInterval","53")
        command_check_interval.set_use_fallback_polling_interval(False)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 53)

    def test_interval_increment_when_use_fallback_polling_interval_set_to_False(self):
        #uses default command poll
        config = mcsrouter.utils.config.Config()
        command_check_interval = mcsrouter.mcs.CommandCheckInterval(config)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 20)


        config.set("pushFallbackPollInterval", "4400")
        config.set("commandPollingDelay","53")
        config.set("flagsPollingInterval","83")
        command_check_interval.set_use_fallback_polling_interval(False)
        command_check_interval.set()
        self.assertEqual(command_check_interval.get(), 53)
        command_check_interval.increment()
        # expect the interval to be incremented.
        self.assertEqual(command_check_interval.get(), (53 + 53))

    def test_interval_will_revert_to_mcs_default_polling_when_use_fallback_polling_is_set_to_False(self):
        config = mcsrouter.utils.config.Config()
        config.set("pushFallbackPollInterval", "5500")
        config.set("commandPollingDelay","53")
        config.set("flagsPollingInterval","53")

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
