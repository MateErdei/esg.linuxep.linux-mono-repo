import unittest
import os
import mock
import gzip
import time

from types import SimpleNamespace

import PathManager
import TestMCSResponse
from TestUtils import assert_message_in_logs
from TestUtils import assert_message_not_in_logs

import logging



logger = logging.getLogger("TestMCSConnection")

import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
from mcsrouter.mcsclient.mcs_connection import EnvelopeHandler
import mcsrouter.mcsclient.responses
import mcsrouter.mcsclient.datafeeds

# Stop the mcs router tests writing to disk
class ConfigWithoutSave(mcsrouter.utils.config.Config):
    def save(self, filename=None):
        pass

def dummy_function(*args):
    pass

def raise_exception(message):
    raise AssertionError(message)


def create_test_config(url="http://localhost/foobar"):
    config = ConfigWithoutSave('testConfig.config')
    config.set("MCSURL", url)
    config.set("MCSID", "")
    config.set("MCSPassword", "")
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
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5", "")
        self.assertEqual(actual, expected)

    def test_user_agent_constructor_unknown_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5", "unknown")
        self.assertEqual(actual, expected)

    def test_user_agent_constructor_none_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions "
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5", None)
        self.assertEqual(actual, expected)

    def test_user_agent_constructor_reg_token(self):
        expected = "Sophos MCS Client/5 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5", "Foobar")
        self.assertEqual(actual, expected)

    def test_user_agent_constructor_diff_platform(self):
        expected = "Sophos MCS Client/5 MyPlatform sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("5", "Foobar", "MyPlatform")
        self.assertEqual(actual, expected)

    def test_user_agent_constructor_diff_version(self):
        expected = "Sophos MCS Client/99 Linux sessions regToken"
        actual = mcsrouter.mcsclient.mcs_connection.create_user_agent("99", "Foobar")
        self.assertEqual(actual, expected)

    @mock.patch(__name__ + ".dummy_function")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id", return_value="")
    def test_send_responses_logs_warning_with_empty_body(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        response = SimpleNamespace(m_json_body_size=0, m_correlation_id="ABC123abc", remove_response_file=dummy_function)
        responses = [response]
        self.assertEqual(len(responses), 1)
        with self.assertLogs(level="WARNING") as logs:
            mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)
        assert_message_in_logs("Empty response (Correlation ID: ABC123abc). Not sending", logs.output, log_level="WARNING")
        self.assertFalse(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.called)
        #dummy_function plays the roll of response.remove_response_file
        self.assertEqual(dummy_function.call_count, 1)

    @mock.patch(__name__ + ".dummy_function")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id", return_value="")
    def test_send_responses_sends_response_when_body_valid(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        response = SimpleNamespace(m_json_body_size=12, remove_response_file=dummy_function)
        responses = [response]
        self.assertEqual(len(responses), 1)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.called)
        #dummy_function plays the roll of response.remove_response_file
        self.assertEqual(dummy_function.call_count, 1)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request", return_value=("header", "body"))
    def test_send_responses_does_not_stop_sending_if_one_response_fails(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        sucessful_get_command_path = lambda endpoint_id: "/responses/endpoint/testendpointid/app_id/LiveQuery/correlation_id/ABC123abc/"
        unsuccessful_get_command_path = lambda endpoint_id: raise_exception("Induced Exception")
        bad_response = SimpleNamespace(m_json_body_size=12, get_command_path=unsuccessful_get_command_path, remove_response_file=dummy_function, m_app_id="bad_app_id", m_correlation_id="bad_correlation_id")
        good_response = SimpleNamespace(m_json_body_size=12, get_command_path=sucessful_get_command_path, remove_response_file=dummy_function, m_app_id="good_app_id", m_correlation_id="good_correlation_id", m_gzip_body_size=12, m_json_body = '{"hello": "world"}')

        responses = [bad_response, good_response]
        with self.assertLogs(level="ERROR") as error_logs:
            mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)

        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request.call_count, 1)
        self.assertEqual(len(error_logs.output), 1)
        assert_message_in_logs("Failed to send response (bad_app_id : bad_correlation_id) : Induced Exception", error_logs.output, log_level="ERROR")

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request", return_value=("header", "body"))
    def test_send_live_query_response_with_id(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        json_body = '{"hello": "world"}'
        dummy_get_command_path = lambda endpoint_id: "/responses/endpoint/testendpointid/app_id/LiveQuery/correlation_id/ABC123abc/"
        response = SimpleNamespace(m_json_body_size=12, m_json_body=json_body, m_gzip_body_size=len(json_body), get_command_path=dummy_get_command_path)
        body = mcs_connection.send_live_query_response_with_id(response)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request.call_count, 1)
        self.assertEqual(body, "body")

    @mock.patch("logging.Logger.info")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.EnvelopeHandler._trim_body_from_response_request",
                side_effect=mcsrouter.mcsclient.mcs_connection.EnvelopeHandler._trim_body_from_response_request)
    def test_send_request_does_not_log_response_bodies(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        dummy_path = "/responses/endpoint/testendpointid/app_id/LiveQuery/correlation_id/ABC123abc/"
        dummy_body = gzip.compress(bytes("body", "utf-8"))
        envelope_handler = EnvelopeHandler()
        message = mcs_connection._build_request_string("POST", dummy_path, dummy_body)
        envelope_handler.set_request(message)
        self.assertEqual(logging.Logger.info.call_count, 1)
        expected_args = mock.call("POST {}".format(dummy_path))
        self.assertEqual(logging.Logger.info.call_args, expected_args)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.EnvelopeHandler._trim_body_from_response_request.called)


    @mock.patch("mcsrouter.mcsclient.mcs_connection.EnvelopeHandler._trim_body_from_response_request")
    def test_send_request_does_not_trim_non_response_messages(self, *mockargs):
        message = "GET /policy/application/ALC/INITIAL_ALC_POLICY_ID"
        envelope_handler = EnvelopeHandler()
        envelope_handler.set_request(message)
        self.assertFalse(mcsrouter.mcsclient.mcs_connection.EnvelopeHandler._trim_body_from_response_request.called)

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

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id")
    @mock.patch("os.remove")
    def test_mcs_drops_response_when_receiving_500_from_central(self, *mockargs):

        side_effects = (mcsrouter.mcsclient.mcs_connection.MCSHttpInternalServerErrorException(500, "header", "body"),
                        mcsrouter.mcsclient.mcs_connection.MCSHttpServiceUnavailableException(503, "header", "body"))
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.side_effect=side_effects
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        bad_response1 = mcsrouter.mcsclient.responses.Response("fake/path500", "bad_app_id500", "bad_corellation_id500", time.time(), "bad_body500")
        bad_response2 = mcsrouter.mcsclient.responses.Response("fake/path503", "bad_app_id503", "bad_corellation_id503", time.time(), "bad_body503")

        with mock.patch("os.path.isfile", return_value=True) as isfile_mock:
            responses = [bad_response1, bad_response2]
            with self.assertLogs(level="DEBUG") as logs:
                mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)

        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.call_count, 2)
        self.assertEqual(len(logs.output), 6)
        self.assertEqual(isfile_mock.call_args_list, [mock.call("fake/path500"), mock.call("fake/path503")])
        assert_message_in_logs("Failed to send response (bad_app_id500 : bad_corellation_id500) : MCS HTTP Error: code=500", logs.output, log_level="ERROR")
        assert_message_in_logs("Discarding response 'bad_corellation_id500' due to rejection by central", logs.output, log_level="DEBUG")
        assert_message_in_logs("Removed bad_app_id500 response file: fake/path500", logs.output, log_level="DEBUG")
        assert_message_in_logs("Failed to send response (bad_app_id503 : bad_corellation_id503) : MCS HTTP Error: code=503", logs.output, log_level="ERROR")
        assert_message_in_logs("Discarding response 'bad_corellation_id503' due to rejection by central", logs.output, log_level="DEBUG")
        assert_message_in_logs("Removed bad_app_id503 response file: fake/path503", logs.output, log_level="DEBUG")


    @mock.patch('mcsrouter.sophos_https.socket.create_connection')
    def test_try_create_connection_logs_socket_error(self, mock_connect):

        def throw_socket_timeout(address, timeout, source_address):
            import socket
            raise socket.timeout

        mock_connect.side_effect = throw_socket_timeout

        policy_config = mcsrouter.utils.config.Config()
        with mock.patch("os.path.isfile", return_value=True) as isfile_mock:
            connection = mcsrouter.mcsclient.mcs_connection.MCSConnection(config=policy_config)

        class NoneHostFakeProxy(object):
            def auth_header(self):
                return "Fake Auth Header"

            def host(self):
                return None

            def username(self):
                return "username"

            def password(self):
                return "password"

            def port(self):
                return 8080

        fake_proxy = NoneHostFakeProxy()
        with self.assertLogs(level="DEBUG") as logs:
            connection._try_create_connection(proxy=fake_proxy, host="localhost", port=443)
        assert_message_in_logs("WARNING:mcsrouter.mcsclient.mcs_connection:Failed direct connection to localhost:443", logs.output, log_level="WARNING")
        assert_message_not_in_logs("Traceback", logs.output)

    # @mock.patch(__name__ + ".dummy_function")
    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result", return_value="")
    def test_send_datafeeds_sends_result_when_body_valid(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        some_time_in_the_future = "2601033100"
        datafeed_container = mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id)
        datafeed_container.add_datafeed_result("filepath", feed_id, some_time_in_the_future, content)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeeds(mcs_connection, datafeed_container)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result.called)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result", return_value="")
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeeds._datafeed_result_is_alive", return_value=False)
    def test_send_datafeeds_does_not_send_old_result_files(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        some_time_in_the_future = "2601033100"
        datafeed_container = mcsrouter.mcsclient.datafeeds.Datafeeds("feed_id")
        datafeed_container.add_datafeed_result("filepath", feed_id, some_time_in_the_future, content)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeeds(mcs_connection, datafeed_container)
        self.assertFalse(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result.called)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result", return_value="")
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeeds._datafeed_result_is_too_large", return_value=True)
    def test_send_datafeeds_does_not_send_result_files_that_are_too_large(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        some_time_in_the_future = "2601033100"
        datafeed_container = mcsrouter.mcsclient.datafeeds.Datafeeds("feed_id")
        datafeed_container.add_datafeed_result("filepath", feed_id, some_time_in_the_future, content)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeeds(mcs_connection, datafeed_container)
        self.assertFalse(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result.called)


    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result", return_value="")
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeed.get_json_body_size", return_value=6)
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeeds.get_max_backlog", return_value=20)
    def test_send_datafeeds_prunes_backlog_before_sending(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        some_time_in_the_future = 2601033100
        datafeed_container = mcsrouter.mcsclient.datafeeds.Datafeeds("feed_id")
        for i in range(4):
            timestamp = str(some_time_in_the_future + i)
            file_path = f"{feed_id}-{timestamp}.json"
            self.assertTrue(datafeed_container.add_datafeed_result(file_path, feed_id, timestamp, content))
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeeds(mcs_connection, datafeed_container)
        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result.call_count, 3)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result", return_value="")
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeed.get_json_body_size", return_value=6)
    @mock.patch("mcsrouter.mcsclient.datafeeds.Datafeeds.get_max_backlog", return_value=20)
    def test_send_datafeeds_prunes_backlog_before_sending(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        feed_id = "feed_id"
        content = '{key1: "value1", key2: "value2"}'
        some_time_in_the_future = 2601033100
        datafeed_container = mcsrouter.mcsclient.datafeeds.Datafeeds("feed_id")
        for i in range(4):
            timestamp = str(some_time_in_the_future + i)
            file_path = f"{feed_id}-{timestamp}.json"
            self.assertTrue(datafeed_container.add_datafeed_result(file_path, feed_id, timestamp, content))
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeeds(mcs_connection, datafeed_container)
        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_datafeed_result.call_count, 3)


if __name__ == '__main__':
    unittest.main()
