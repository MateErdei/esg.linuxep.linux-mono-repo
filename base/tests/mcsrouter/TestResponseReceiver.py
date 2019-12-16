#!/usr/bin/env python

import os
import mock
from mock import patch, mock_open
import json
import unittest
import logging
logger = logging.getLogger("TestResponseReceiver")

import PathManager

import mcsrouter.adapters.response_receiver as response_receiver

DUMMY_TIMESTAMP=1567677734.4998658

@mock.patch('os.remove', return_value="")
@mock.patch('os.path.getmtime', return_value=DUMMY_TIMESTAMP)
class TestResponseReceiver(unittest.TestCase):
    def test_validate_string_as_json_throws_on_empty_string(self, *mockargs):
        self.assertRaises(json.decoder.JSONDecodeError, response_receiver.validate_string_as_json, "")

    def test_validate_string_as_json_does_not_throw_on_valid_string(self, *mockargs):
        response_receiver.validate_string_as_json('{"hello": "world"}')

    @mock.patch('os.listdir', return_value=["EDR_ABC123abc_response.txt", "EDR_ABC123abc_command.json", "EDR_!a?b£c_response.json", "123_ABC123abc_response.json"])
    def test_receive_ignores_invalid_filenames(self, *mockarg):
        for response in response_receiver.receive():
            raise AssertionError
        self.assertFalse(os.path.getmtime.called)
        self.assertTrue(os.remove.called)

    @mock.patch('os.listdir', return_value=["EDR_ABC123abc_response.json", "MTR_XYZ987xyz_response.json"])
    @mock.patch('builtins.open')
    @mock.mock_open(read_data='{"hello": "world"}')
    def test_receive_yields_correct_output(self, *mockargs):
        response1 = next(response_receiver.receive())
        self.assertEqual(response1[0], "EDR")
        self.assertEqual(response1[1], "ABC123abc")
        self.assertEqual(response1[2], DUMMY_TIMESTAMP)
        self.assertEqual(response1[3], '{"hello": "world"}')

        response2 = next(response_receiver.receive())
        self.assertEqual(response2[0], "MTR")
        self.assertEqual(response2[1], "XYZ987xyz")
        self.assertEqual(response2[2], DUMMY_TIMESTAMP)
        self.assertEqual(response2[3], '{"hello": "world"}')

        self.assertTrue(open.call_count, 2)

if __name__ == '__main__':
    unittest.main()


