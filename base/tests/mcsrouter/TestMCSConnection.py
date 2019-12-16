
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

import logging
logger = logging.getLogger("TestMCSConnection")

import mcsrouter.utils.config
import mcsrouter.mcsclient.mcs_connection
import mcsrouter.mcsclient.mcs_exception
import mcsrouter.mcsclient.mcs_connection

# Stop the mcs router tests writing to disk
class ConfigWithoutSave(mcsrouter.utils.config.Config):
    def save(self, filename=None):
        pass


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

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id", return_value="")
    def test_send_responses_logs_warning_with_empty_body(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        response = SimpleNamespace(m_json_body_size=0, m_correlation_id="ABC123abc")
        responses = [response]
        self.assertEqual(len(responses), 1)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)
        self.assertFalse(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.called)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id", return_value="")
    def test_send_responses_sends_response_when_body_valid(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        response = SimpleNamespace(m_json_body_size=12)
        responses = [response]
        self.assertEqual(len(responses), 1)
        mcsrouter.mcsclient.mcs_connection.MCSConnection.send_responses(mcs_connection, responses)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.MCSConnection.send_live_query_response_with_id.called)

    @mock.patch("mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request", return_value=("header", "body"))
    def test_send_live_query_response_with_id(self, *mockargs):
        mcs_connection = TestMCSResponse.dummyMCSConnection()
        gzip_body = gzip.compress(bytes("body", "utf-8"))
        dummy_get_command_path = lambda endpoint_id: "/responses/endpoint/testendpointid/app_id/EDR/correlation_id/ABC123abc/"
        response = SimpleNamespace(m_json_body_size=12, m_gzip_body=gzip_body, m_gzip_body_size=len(gzip_body), get_command_path=dummy_get_command_path)
        body = mcs_connection.send_live_query_response_with_id(response)
        self.assertTrue(mcsrouter.mcsclient.mcs_connection.MCSConnection._MCSConnection__request.call_count, 1)
        self.assertEqual(body, "body")

if __name__ == '__main__':
    unittest.main()
