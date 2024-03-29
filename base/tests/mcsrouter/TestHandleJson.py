#!/usr/bin/env python

import PathManager

import mcsrouter.utils.handle_json

import unittest
from unittest import mock
import json
from unittest.mock import patch, mock_open
import logging
import time

DUMMY_TIMESTAMP = 1613400109.7271419


class TestHandleJson(unittest.TestCase):
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data="")
    def test_read_datafeed_tracker_return_default_if_file_is_empty(self, *mockargs):
        expected = {"size":0, "time_sent":int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=False)
    def test_read_datafeed_tracker_return_default_if_file_does_not_exist(self, *mockargs):
        expected = {"size":0, "time_sent":int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    # mocking time we don't expect to get to prove that the files value is used
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":42,"time_sent":{int(DUMMY_TIMESTAMP)-200}}}""")
    def test_read_datafeed_tracker_returns_content_of_file(self, *mockargs):
        expected = {"size": 42, "time_sent": int(DUMMY_TIMESTAMP)-200}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":"hello","time_sent":{int(DUMMY_TIMESTAMP)-200}}}""")
    def test_read_datafeed_tracker_ignores_non_int_size_value_in_json(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)-200}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":"hello","time_sent":"notatimestamp"}}""")
    def test_read_datafeed_tracker_ignores_non_int_time_sent_value_in_json(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":48,"time_sent":-34}}""")
    def test_read_datafeed_tracker_handles_negative_timestamp(self, *mockargs):
        expected = {"size": 48, "time_sent": -34}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":-48,"time_sent": {int(DUMMY_TIMESTAMP)}}}""")
    def test_read_datafeed_tracker_handles_negative_size(self, *mockargs):
        expected = {"size": -48, "time_sent": int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":"hello","time_sent":
    100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000}}""")
    def test_read_datafeed_tracker_large_time(self, *mockargs):
        expected = {"size": 0, "time_sent": 100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":b'sss',"time_sent":b'222'}}""")
    def test_read_datafeed_tracker_binary(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"size":0,"time_sent":{int(DUMMY_TIMESTAMP)-200},"randomfield":"blah"}}""")
    def test_read_datafeed_tracker_handles_extra_field(self, *mockargs):
        expected = {"randomfield": "blah", "size": 0, "time_sent": int(DUMMY_TIMESTAMP)-200}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""""")
    def test_read_datafeed_tracker_handles_empty_json(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{,}}""")
    def test_read_datafeed_tracker_handles_invalid_json(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=f"""{{"time_sent":{int(DUMMY_TIMESTAMP)-200}}}""")
    def test_read_datafeed_tracker_handles_missing_size_value_in_json(self, *mockargs):
        expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)-200}
        data = mcsrouter.utils.handle_json.read_datafeed_tracker()
        self.assertEqual(data, expected)

    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    @mock.patch('os.path.exists', return_value=True)
    def test_read_datafeed_tracker_handles_permission_error(self, *mockargs):
        with patch('builtins.open') as mock_open:
            mock_open.side_effect = PermissionError
            expected = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
            data = mcsrouter.utils.handle_json.read_datafeed_tracker()
            self.assertEqual(data, expected)

    @mock.patch("logging.Logger.info")
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_does_not_throw_on_permission_error(self, *mockargs):
        with patch('builtins.open') as mock_open:
            mock_open.side_effect = PermissionError
            from_file = {"size": 3, "time_sent": 0}
            mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)


    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_handles_binary_size(self, *mockargs):
        from_file = {"size": b'string', "time_sent": 0}
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)

    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_handles_binary_size(self, *mockargs):
        from_file = {"size": 400, "time_sent": b'not time'}
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)

    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_logs_when_it_has_been_more_than_24_hours_since_last_log(self, *mockargs):
        from_file = {"size": 3, "time_sent": 0}
        expected_time_last_sent = round(time.time()/3600, 1)
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)
        self.assertEqual(logging.Logger.info.call_args_list[-1],
                         mock.call(f"In the past {expected_time_last_sent}h we have sent 0.045kB of scheduled query data to Central"))


    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_does_not_log_when_it_has_been_less_than_24_hours_since_last_log(self, *mockargs):
        exactly_23h_59m_59s_ago = int(DUMMY_TIMESTAMP)-((24*60*60)-1)
        from_file = {"size": 3, "time_sent": exactly_23h_59m_59s_ago}
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)
        logging.Logger.info.assert_not_called()

    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_logs_when_it_has_been_exactly_24_hours_since_last_log(self, *mockargs):
        exactly_24h_ago = int(DUMMY_TIMESTAMP)-((24*60*60))
        from_file = {"size": 3, "time_sent": exactly_24h_ago}
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 47)
        self.assertEqual(logging.Logger.info.call_args_list[-1],
                         mock.call(f"In the past 24.0h we have sent 0.05kB of scheduled query data to Central"))

    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('json.dump')
    @mock.patch('os.chmod')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_increases_size_of_data_sent_and_does_not_change_timestamp_when_less_than_24h(self, *mockargs):
        from_file = {"size": 3, "time_sent": int(DUMMY_TIMESTAMP-100)}
        expected_file = {"size": 45, "time_sent": int(DUMMY_TIMESTAMP-100)}
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 42)
        logging.Logger.info.assert_not_called()
        json.dump.assert_called_once_with(expected_file, mock.ANY)

    @mock.patch("logging.Logger.info")
    @mock.patch('builtins.open', new_callable=mock_open)
    @mock.patch('os.chmod')
    @mock.patch('json.dump')
    @mock.patch('time.time', return_value=DUMMY_TIMESTAMP)
    def test_update_datafeed_tracker_resets_size_to_zero_after_sending(self, *mockargs):
        from_file = {"size": 3, "time_sent": 0}
        expected_file = {"size": 0, "time_sent": int(DUMMY_TIMESTAMP)}
        expected_time_last_sent = round(time.time()/3600, 1)
        mcsrouter.utils.handle_json.update_datafeed_tracker(from_file, 47)
        self.assertEqual(logging.Logger.info.call_args_list[-1],
                         mock.call(f"In the past {expected_time_last_sent}h we have sent 0.05kB of scheduled query data to Central"))
        json.dump.assert_called_once_with(expected_file, mock.ANY)
