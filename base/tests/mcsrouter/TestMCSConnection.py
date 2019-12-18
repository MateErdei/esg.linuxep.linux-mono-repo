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

    def testQueryWillProcessCompleteCommands(self):
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


    def testQueryCommandRejectCommandWithoutID(self):
        mcs_connection=FakeMCSConnection("""<command>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command. Missing required field: 'id'")

    def testQueryCommandRejectCommandWithEmptyID(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id></id>
        <appId>LiveQuery</appId>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command: Required field 'id' is empty")

    def testQueryCommandRejectCommandWithoutAppId(self):
        mcs_connection=FakeMCSConnection("""<command>
        <id>anyid</id>
        <creationTime>FakeTime</creationTime>
        <ttl>PT10000S</ttl>
        <body>anybody</body>
      </command>""")
        with self.assertLogs() as logs:
            mcs_connection.query_commands(['ALC', 'LiveQuery'])
        self._log_contains(logs.output, "Invalid command. Missing required field: 'appId'")


    def testQueryCommandRejectCommandWithEmptyID(self):
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


    def testQueryCommandRejectCommandWithoutBody(self):
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
