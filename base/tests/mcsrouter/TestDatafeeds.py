import os
import unittest
import mock
import time

import sys
import json
import builtins
import datetime

import logging
logger = logging.getLogger("TestDatafeeds")

import PathManager

# import mcsrouter.mcs
#
# import mcsrouter.mcsclient.mcs_exception
# import mcsrouter.mcsclient.mcs_connection
# import mcsrouter.mcsclient.mcs_commands as mcs_commands
# import mcsrouter.adapters.generic_adapter as generic_adapter
# import mcsrouter.adapters.agent_adapter as agent_adapter
# import mcsrouter.utils.target_system_manager
import modules.mcsrouter.mcsrouter.mcsclient.datafeeds
class TestDatafeeds(unittest.TestCase):
    def test_x(self):
        feed_id = "feed_id_1"
        content = '{key1: "value1", key2: "value2"}'
        datafeeds = modules.mcsrouter.mcsrouter.mcsclient.datafeeds.Datafeeds(feed_id, 10, 10, 10, 10, 10)
        datafeeds.add_datafeed_result("/tmp/filepath1", feed_id, "1601033100", content)
        datafeeds.add_datafeed_result("/tmp/filepath3", feed_id, "1601033300", content)
        datafeeds.add_datafeed_result("/tmp/filepath2", feed_id, "1601033200", content)
        datafeed_results = datafeeds.get_datafeeds()
        self.assertEqual(len(datafeed_results), 3)

        #     self.m_file_path = file_path
        # self.m_datafeed_id = datafeed_id_name
        # self.m_creation_time = creation_time
        # self.m_json_body = body
        # self.m_gzip_body = self._get_compressed_json()
        # self.m_json_body_size = self._get_decompressed_body_size()
        # self.m_gzip_body_size = self._get_compressed_body_size()

        datafeeds.sort_oldest_to_newest()
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033100")
        self.assertEqual(datafeed_results[0].m_json_body, content)
        self.assertEqual(datafeed_results[1].m_file_path, "/tmp/filepath2")
        self.assertEqual(datafeed_results[1].m_creation_time, "1601033200")
        self.assertEqual(datafeed_results[1].m_json_body, content)
        self.assertEqual(datafeed_results[2].m_file_path, "/tmp/filepath3")
        self.assertEqual(datafeed_results[2].m_creation_time, "1601033300")
        self.assertEqual(datafeed_results[2].m_json_body, content)

        datafeeds.sort_newest_to_oldest()
        self.assertEqual(datafeed_results[0].m_file_path, "/tmp/filepath3")
        self.assertEqual(datafeed_results[0].m_creation_time, "1601033300")
        self.assertEqual(datafeed_results[1].m_file_path, "/tmp/filepath2")
        self.assertEqual(datafeed_results[1].m_creation_time, "1601033200")
        self.assertEqual(datafeed_results[2].m_file_path, "/tmp/filepath1")
        self.assertEqual(datafeed_results[2].m_creation_time, "1601033100")


    # def test_multiple_responses_can_be_added_and_retrieved_through_responses_object(self, *mockargs):
    #     responses = mcsrouter.mcsclient.responses.Responses()
    #     responses.add_response(DUMMY_PATH, "LiveQuery", "correlation_id", "timestamp", EXAMPLE_BODY)
    #     responses.add_response(DUMMY_PATH, "LiveQuery", "correlation_id2", "timestamp2", EXAMPLE_BODY)
    #     responses.add_response(DUMMY_PATH, "LiveQuery", "correlation_id3", "timestamp3", EXAMPLE_BODY)
    #     array_of_responses = responses.get_responses()
    #     self.assertEqual(len(array_of_responses), 3)
    #     response1 = array_of_responses[0]
    #     response2 = array_of_responses[1]
    #     response3 = array_of_responses[2]
    #     self.validate_response_object(response1, "LiveQuery", "correlation_id", "timestamp")
    #     self.validate_response_object(response2, "LiveQuery", "correlation_id2", "timestamp2")
    #     self.validate_response_object(response3, "LiveQuery", "correlation_id3", "timestamp3")