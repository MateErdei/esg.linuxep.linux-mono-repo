import unittest
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

class FakeMCSConnection(mcsrouter.mcsclient.mcs_connection.MCSConnection):
    def __init__(self, command_xml):
        self.command_xml = command_xml

    def send_message_with_id(self, url):
        logger = logging.getLogger("test")
        logger.info("Inside send message with id")

        return self.command_xml


class TestMCSConnection(unittest.TestCase):
    def _log_contains(self, logs, message):
        for log_message in logs:
            if message in log_message:
                return
        raise AssertionError( "Messsage: {}, not found in logs: {}".format(message, logs))

    def _log_does_not_contain(self, logs, message):
        for log_message in logs:
            if message in log_message:
                raise AssertionError( "Messsage: {}, found in logs: {}".format(message, logs))


    def tearDown(self):
        try:
            os.remove("testConfig.config")
        except Exception:
            pass

    def test_user_agent_constructor_empty_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","")
        self.assertEqual(actual,expected)

    def test_user_agent_constructor_unknown_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","unknown")
        self.assertEqual(actual,expected)

    def test_user_agent_constructor_none_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5",None)
        self.assertEqual(actual,expected)

    def test_user_agent_constructor_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","Foobar")
        self.assertEqual(actual,expected)

    def test_user_agent_constructor_diff_platform(self):
        expected = "Sophos MCS Client/5 MyPlatform sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5","Foobar","MyPlatform")
        self.assertEqual(actual,expected)

    def test_user_agent_constructor_diff_version(self):
        expected = "Sophos MCS Client/99 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("99","Foobar")
        self.assertEqual(actual,expected)

    def test_query_will_process_complete_commands(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id>anyid</id>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_does_not_contain(logs.output, 'error')


    def test_query_command_reject_command_without_id(self):
        mcs_connection=FakeMCSConnection("""<command>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command. Missing required field: 'id'")

    def test_query_command_reject_command_with_empty_id(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id></id>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command: Required field 'id' must not be empty")

    def test_query_command_reject_command_withhout_app_id(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id>anyid</id>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command. Missing required field: 'appId'")


    def test_query_command_reject_command_with_empty_app_id(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id>anyid</id>
        <appId></appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command: Required field 'AppId' is empty")


    def test_query_command_reject_command_without_body(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id>anyid</id>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command. Missing required field: 'body'")



# except ImportError:
#     print >>sys.stderr,sys.path
#     class TestMCSConnection(unittest.TestCase):
#         pass

if __name__ == '__main__':
    unittest.main()
