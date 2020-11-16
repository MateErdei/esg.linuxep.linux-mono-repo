#!/usr/bin/env python

import os
import mock
from mock import patch, mock_open
import json
import unittest
import logging
logger = logging.getLogger("TestDatafeedReceiver")

import PathManager

import mcsrouter.adapters.datafeed_receiver as datafeed_receiver

DATAFEED_DIR = "/tmp/sophos-spl/base/mcs/datafeed"
DUMMY_TIMESTAMP = 1567677734  # '2019-09-05T10:02:14'

INVALID_BODY=b"""{
  not a valid json
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 1,
        "errorMessage": "no such table: ophos_process_journal "
    }
} """

@mock.patch('os.path.isfile', return_value=True)
@mock.patch('os.remove', return_value="")
class TestDatafeedReceiver(unittest.TestCase):
    def test_validate_string_as_json_throws_on_empty_string(self, *mockargs):
        self.assertRaises(json.decoder.JSONDecodeError, datafeed_receiver.validate_string_as_json, "")

    def test_validate_string_as_json_does_not_throw_on_valid_string(self, *mockargs):
        datafeed_receiver.validate_string_as_json('{"hello": "world"}')

    def test_validate_string_as_json_throws_on_invalid_json(self, *mockargs):
        self.assertRaises(json.decoder.JSONDecodeError, datafeed_receiver.validate_string_as_json, INVALID_BODY)

    @mock.patch("logging.Logger.error")
    @mock.patch('os.listdir', return_value=[f"scheduled_query-{DUMMY_TIMESTAMP}.json"])
    @mock.patch('mcsrouter.utils.path_manager.datafeed_dir', return_value=DATAFEED_DIR)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=INVALID_BODY)
    def test_receive_removes_invalid_json_file(self, *mockargs):
        for response in datafeed_receiver.receive():
            raise AssertionError
        self.assertEqual(os.remove.call_count, 1)
        self.assertEqual(os.remove.call_args, mock.call(os.path.join(DATAFEED_DIR, f"scheduled_query-{DUMMY_TIMESTAMP}.json")))
        self.assertTrue(logging.Logger.error.called)

    @mock.patch('os.listdir',
                return_value=[f"scheduled_query-{DUMMY_TIMESTAMP}.txt",
                              f"scheduled-query-{DUMMY_TIMESTAMP}.json",
                              f"scheduled_query{DUMMY_TIMESTAMP}.json",
                              "scheduled_query-ABC123abcresponse.json",
                              f"scheduled_query-{DUMMY_TIMESTAMP}",
                              f"{DUMMY_TIMESTAMP}.json"
                              f"scheduled_query-{DUMMY_TIMESTAMP}jsonasdhfgthert"])
    def test_receive_ignores_invalid_filenames(self, *mockarg):
        for response in datafeed_receiver.receive():
            raise AssertionError

    @mock.patch('os.listdir', return_value=[f"scheduled_query-{DUMMY_TIMESTAMP}.json", f"scheduled_query2-{DUMMY_TIMESTAMP}.json"])
    @mock.patch('builtins.open', new_callable=mock_open, read_data='{"hello": "world"}')
    def test_receive_yields_correct_output(self, *mockargs):
        lst = list(datafeed_receiver.receive())

        datafeed1 = lst[0]
        self.assertEqual(datafeed1[0], os.path.join(DATAFEED_DIR, f"scheduled_query-{DUMMY_TIMESTAMP}.json"))
        self.assertEqual(datafeed1[1], "scheduled_query")
        self.assertEqual(datafeed1[2], f"{DUMMY_TIMESTAMP}")
        self.assertEqual(datafeed1[3], '{"hello": "world"}')

        datafeed2 = lst[1]
        self.assertEqual(datafeed2[0], os.path.join(DATAFEED_DIR, f"scheduled_query2-{DUMMY_TIMESTAMP}.json"))
        self.assertEqual(datafeed2[1], "scheduled_query2")
        self.assertEqual(datafeed2[2], f"{DUMMY_TIMESTAMP}")
        self.assertEqual(datafeed2[3], '{"hello": "world"}')

        self.assertTrue(open.call_count, 2)
        self.assertFalse(os.remove.called)

    @mock.patch("logging.Logger.error")
    @mock.patch('os.listdir', return_value=[f"scheduled_query-{DUMMY_TIMESTAMP}.json"])
    def test_receive_logs_error_when_unable_to_read_file(self, *mockargs):
        with patch('builtins.open') as mock_open:
            mock_open.side_effect = OSError
            for response in datafeed_receiver.receive():
                raise AssertionError

            self.assertTrue(open.call_count, 1)
            self.assertFalse(os.remove.called)
            self.assertTrue(logging.Logger.error.called)


    def test_remove_datafeed_file_successfully_removes_file(self, *mockargs):
        datafeed_receiver.remove_datafeed_file("/fake/file/path")
        self.assertTrue(os.remove.called)

    @mock.patch("logging.Logger.warning")
    def test_remove_datafeed_file_logs_warning_when_raises_oserror(self, *mockargs):
        with patch('os.remove') as fs:
            fs.side_effect = OSError
            datafeed_receiver.remove_datafeed_file("/fake/file/path")
            self.assertTrue(os.remove.called)
            self.assertTrue(logging.Logger.warning.called)



if __name__ == '__main__':
    unittest.main()


