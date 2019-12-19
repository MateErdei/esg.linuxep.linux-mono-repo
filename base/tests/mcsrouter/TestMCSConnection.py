
from __future__ import absolute_import,print_function,division,unicode_literals

import unittest
import sys
import json
import os
import mock
import gzip

from types import SimpleNamespace

import PathManager
import TestMCSResponse
from TestUtils import assert_message_in_logs

import logging
logger = logging.getLogger("TestMCSConnection")

import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection
from mcsrouter.mcsclient.mcs_connection import EnvelopeHandler

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

class TestMCSConnection(unittest.TestCase):
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
        good_response = SimpleNamespace(m_json_body_size=12, get_command_path=sucessful_get_command_path, remove_response_file=dummy_function, m_app_id="good_app_id", m_correlation_id="good_correlation_id", m_gzip_body_size=12, m_gzip_body = "body")

        responses = [bad_response, good_response]
        with self.assertLogs(level="ERROR") as error_logs:
            mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)

        self.assertEqual(mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request.call_count, 1)
        self.assertEqual(len(error_logs.output), 1)
        assert_message_in_logs("Failed to send response (bad_app_id : bad_correlation_id) : Induced Exception", error_logs.output, log_level="ERROR")

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request", return_value=("header", "body"))
    def test_send_live_query_response_with_id(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        gzip_body = gzip.compress(bytes("body", "utf-8"))
        dummy_get_command_path = lambda endpoint_id: "/responses/endpoint/testendpointid/app_id/LiveQuery/correlation_id/ABC123abc/"
        response = SimpleNamespace(m_json_body_size=12, m_gzip_body=gzip_body, m_gzip_body_size=len(gzip_body), get_command_path=dummy_get_command_path)
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


if __name__ == '__main__':
    unittest.main()
