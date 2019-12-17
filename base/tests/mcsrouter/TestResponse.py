#!/usr/bin/env python



import unittest
import mock
import sys
import time

import gzip
import logging
logger = logging.getLogger("TestResponse")

import PathManager

import mcsrouter.mcsclient.responses
import mcsrouter.utils.timestamp as timestamp

EXAMPLE_BODY = """{
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "durationMillis": 32,
        "sizeBytes": 0,
        "rows": 0,
        "errorCode": 0,
        "errorMessage": "OK"
    },
    "columnMetaData": [
        {"name": "pathname", "type": "TEXT"},
        {"name": "sophosPID", "type": "TEXT"},
        {"name": "start_time", "type": "BIGINT"}
    ]
}
"""

EXAMPLE_BODY_SIZE = len(EXAMPLE_BODY)
EXAMPLE_BODY_COMPRESSED_SIZE = len(gzip.compress(bytes(EXAMPLE_BODY, "utf-8")))

DUMMY_ENDPOINT_ID = "dummyEndpointID"




class TestResponse(unittest.TestCase):
    def validate_response_object(self,
                                 response,
                                 expected_app_id,
                                 expected_corellation_id,
                                 expected_creation_time,
                                 expected_json_body=EXAMPLE_BODY,
                                 expected_json_body_size=EXAMPLE_BODY_SIZE,
                                 expected_gzip_body_size=EXAMPLE_BODY_COMPRESSED_SIZE):
        self.assertEqual(response.m_app_id, expected_app_id)
        self.assertEqual(response.m_correlation_id, expected_corellation_id)
        self.assertEqual(response.m_creation_time, expected_creation_time)
        self.assertEqual(response.m_json_body, expected_json_body)
        self.assertEqual(response.m_json_body_size, expected_json_body_size)
        self.assertEqual(type(response.m_gzip_body), bytes)
        self.assertEqual(response.m_gzip_body_size, expected_gzip_body_size)
        self.assertEqual(gzip.decompress(response.m_gzip_body).decode("utf-8"), expected_json_body)
        expected_command_path = "/responses/endpoint/{}/app_id/{}/correlation_id/{}/".format(
            DUMMY_ENDPOINT_ID,
            expected_app_id,
            expected_corellation_id)
        self.assertEqual(expected_command_path, response.get_command_path(DUMMY_ENDPOINT_ID))

    def test_response_object_sanity(self):
        response = mcsrouter.mcsclient.responses.Response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        self.validate_response_object(response, "app_id", "correlation_id", "timestamp")

    def test_create_empty_response(self):
        response = mcsrouter.mcsclient.responses.Response("app_id", "correlation_id", "timestamp", "")
        self.assertEqual(response.m_json_body, "")
        self.assertEqual(response.m_json_body_size, 0)
        self.assertEqual(type(response.m_gzip_body), bytes)
        self.assertEqual(response.m_gzip_body_size, 20)
        self.assertEqual(gzip.decompress(response.m_gzip_body).decode("utf-8"), "")

    def test_body_is_preserved(self):
        response = mcsrouter.mcsclient.responses.Response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        self.assertEqual(gzip.decompress(response.m_gzip_body).decode("utf-8"), EXAMPLE_BODY)

    def test_get_command_path_produces_path_in_correct_format(self):
        response = mcsrouter.mcsclient.responses.Response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        expected_command_path = "/responses/endpoint/{}/app_id/{}/correlation_id/{}/".format(
            DUMMY_ENDPOINT_ID,
            "app_id",
            "correlation_id")
        self.assertEqual(expected_command_path, response.get_command_path(DUMMY_ENDPOINT_ID))

    def test_responses_can_be_added_and_retrieved_through_responses_object(self):
        responses = mcsrouter.mcsclient.responses.Responses()
        responses.add_response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        array_of_responses = responses.get_responses()
        self.assertEqual(len(array_of_responses), 1)
        response = array_of_responses[0]
        self.validate_response_object(response, "app_id", "correlation_id", "timestamp")


    def test_multiple_responses_can_be_added_and_retrieved_through_responses_object(self):
        responses = mcsrouter.mcsclient.responses.Responses()
        responses.add_response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        responses.add_response("app_id2", "correlation_id2", "timestamp2", EXAMPLE_BODY)
        responses.add_response("app_id3", "correlation_id3", "timestamp3", EXAMPLE_BODY)
        array_of_responses = responses.get_responses()
        self.assertEqual(len(array_of_responses), 3)
        response1 = array_of_responses[0]
        response2 = array_of_responses[1]
        response3 = array_of_responses[2]
        self.validate_response_object(response1, "app_id", "correlation_id", "timestamp")
        self.validate_response_object(response2, "app_id2", "correlation_id2", "timestamp2")
        self.validate_response_object(response3, "app_id3", "correlation_id3", "timestamp3")

    def test_responses_object_resets(self):
        responses = mcsrouter.mcsclient.responses.Responses()
        # populate  __m_responses
        for i in range(10):
            responses.add_response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        self.assertEqual(len(responses.get_responses()), 10)
        responses.reset()
        self.assertEqual(len(responses.get_responses()), 0)

    def test_responses_can_tell_if_it_has_responses(self):
        responses = mcsrouter.mcsclient.responses.Responses()

        self.assertFalse(responses.has_responses())

        responses.add_response("app_id", "correlation_id", "timestamp", EXAMPLE_BODY)
        self.assertTrue(responses.has_responses())

    @mock.patch('time.time', return_value=1576600337.2105377)
    def test_prune_old_responses_removes_old_response(self, *mockargs):
        responses = mcsrouter.mcsclient.responses.Responses()
        responses.add_response("app_id", "correlation_id", timestamp.timestamp(time.time() - 35), EXAMPLE_BODY)
        self.assertFalse(responses.has_responses())
        self.assertEqual([], responses.get_responses())

    @mock.patch('time.time', return_value=1576600337.2105377)
    def test_prune_old_responses_does_not_remove_current_response(self, *mockargs):
        responses = mcsrouter.mcsclient.responses.Responses()
        responses.add_response("app_id", "correlation_id", timestamp.timestamp(time.time() - 25), EXAMPLE_BODY)
        self.assertTrue(responses.has_responses())
        self.assertEqual(1, len(responses.get_responses()))

    @mock.patch('time.time', return_value=1576600337.2105377)
    def test_prune_old_responses_only_removes_old_response(self, *mockargs):
        responses = mcsrouter.mcsclient.responses.Responses()
        responses.add_response("app_id", "correlation_id", timestamp.timestamp(time.time() - 25), EXAMPLE_BODY)
        responses.add_response("app_id", "correlation_id", timestamp.timestamp(time.time() - 35), EXAMPLE_BODY)
        self.assertEqual(2, len(responses.get_responses()))
        self.assertTrue(responses.has_responses())
        self.assertEqual(1, len(responses.get_responses()))

    @mock.patch('time.time', return_value=1576600337.2105377)
    def test_prune_old_responses_does_nothing_if_no_responses(self, *mockargs):
        responses = mcsrouter.mcsclient.responses.Responses()
        self.assertEqual(0, len(responses.get_responses()))
        self.assertFalse(responses.has_responses())
        self.assertEqual(0, len(responses.get_responses()))


if __name__ == '__main__':
    unittest.main()


