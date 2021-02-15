#!/usr/bin/env python

import PathManager
import mcsrouter.utils.flags as flags
import mcsrouter.utils.write_json

import unittest
import os
import mock
from mock import patch, mock_open
import logging
import time
DUMMY_TIMESTAMP=1613400109.7271419
#logger = logging.getLogger("TestResponse")

class TestFlags(unittest.TestCase):
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="")
    def test_read_datafeed_tracker_return_default_if_file_is_empty(self, *mockargs):
        expected = {"size":0, "time_sent":int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.write_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=False)
    def test_read_datafeed_tracker_return_default_if_file_does_not_exist(self, *mockargs):
        expected = {"size":0, "time_sent":int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.write_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    # mocking time we don't expect to get to prove that the files value is used
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":42,"time_sent":{int(DUMMY_TIMESTAMP)-200}}}""")
    def test_read_datafeed_tracker_returns_content_of_file(self, *mockargs):
        expected = {"size":42, "time_sent":int(DUMMY_TIMESTAMP)-200}
        data = mcsrouter.utils.write_json.read_datafeed_tracker()
        self.assertEqual(data, expected)


    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    def test_update_datafeed_tracker(self, *mockargs):
        expected = {"size": 3, "time_sent": 0}
        mcsrouter.utils.write_json.update_datafeed_tracker(expected, 42)
        self.assertEqual(logging.Logger.info.call_args_list[-1], mock.call("Sent 0.045kB of datafeed to Central since 1970-01-01T00:00:00Z"))

