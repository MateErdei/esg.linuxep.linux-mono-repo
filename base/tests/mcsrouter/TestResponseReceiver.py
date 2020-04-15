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

RESPONSE_DIR = "/tmp/sophos-spl/base/mcs/response"
DUMMY_TIMESTAMP = 1567677734.4998658  # '2019-09-05T10:02:14.499865Z'

INVALID_BODY=b"""{
  not a valid json
    "type": "sophos.mgt.response.RunLiveQuery",
    "queryMetaData": {
        "errorCode": 1,
        "errorMessage": "no such table: ophos_process_journal "
    }
} """

def mutating_open_mock(*args):
    args = open.call_args.args
    if open.call_count > 2:
        mock.mock_open(mock=open, read_data=b'{"hello": "world"}')
        open.call_count -= 1
        return open(*args)
    else:  
        mock.mock_open(mock=open, read_data=b'\xc3\xaep\xa5\x18\x9ajK\x98\x96')
        return_object = open(*args)
        open.call_count -= 1
        open.side_effect = mutating_open_mock
        return return_object

@mock.patch('os.path.isfile', return_value=True)
@mock.patch('os.remove', return_value="")
@mock.patch('os.path.getmtime', return_value=DUMMY_TIMESTAMP)
class TestResponseReceiver(unittest.TestCase):
    def test_validate_string_as_json_throws_on_empty_string(self, *mockargs):
        self.assertRaises(json.decoder.JSONDecodeError, response_receiver.validate_string_as_json, "")

    def test_validate_string_as_json_does_not_throw_on_valid_string(self, *mockargs):
        response_receiver.validate_string_as_json('{"hello": "world"}')

    def test_validate_string_as_json_throws_on_invalid_json(self, *mockargs):
        self.assertRaises(json.decoder.JSONDecodeError, response_receiver.validate_string_as_json, INVALID_BODY)

    @mock.patch("logging.Logger.error")
    @mock.patch('os.listdir', return_value=["LiveQuery_ABC123abc_response.json"])
    @mock.patch('mcsrouter.utils.path_manager.response_dir', return_value=RESPONSE_DIR)
    @mock.patch('builtins.open', new_callable=mock_open, read_data=INVALID_BODY)
    def test_receive_removes_invalid_json_file(self, *mockargs):
        for response in response_receiver.receive():
            raise AssertionError
        self.assertTrue(os.path.getmtime.called)
        self.assertEqual(os.remove.call_count, 1)
        self.assertEqual(os.remove.call_args, mock.call(os.path.join(RESPONSE_DIR, "LiveQuery_ABC123abc_response.json")))
        self.assertTrue(logging.Logger.error.called)

    @mock.patch('os.listdir',
                return_value=["LiveQuery_ABC123abc_response.txt",
                              "LiveQuery_ABC1_23abc_command.json",
                              "LiveQuery!a?b£cresponse.json",
                              "LiveQuery_ABC123abcresponse.json",
                              "LiveQuery_ABC123abc_response",
                              "LiveQuery_ABC123abc_responseOjsonadasdas"])
    def test_receive_ignores_invalid_filenames(self, *mockarg):
        for response in response_receiver.receive():
            raise AssertionError
        self.assertFalse(os.path.getmtime.called)

    @mock.patch('os.listdir', return_value=["LiveQuery_ABC123abc_response.json", "LiveQuery_XYZ987xyz_response.json"])
    @mock.patch('builtins.open', new_callable=mock_open, read_data=b'{"hello": "world"}')
    def test_receive_yields_correct_output(self, *mockargs):
        lst = list(response_receiver.receive())

        response1 = lst[0]
        self.assertEqual(response1[0], os.path.join(RESPONSE_DIR, "LiveQuery_ABC123abc_response.json"))
        self.assertEqual(response1[1], "LiveQuery")
        self.assertEqual(response1[2], "ABC123abc")
        self.assertEqual(response1[3], DUMMY_TIMESTAMP)
        self.assertEqual(response1[4], '{"hello": "world"}')

        response2 = lst[1]
        self.assertEqual(response2[0], os.path.join(RESPONSE_DIR, "LiveQuery_XYZ987xyz_response.json"))
        self.assertEqual(response2[1], "LiveQuery")
        self.assertEqual(response2[2], "XYZ987xyz")
        self.assertEqual(response2[3], DUMMY_TIMESTAMP)
        self.assertEqual(response2[4], '{"hello": "world"}')

        self.assertTrue(open.call_count, 2)
        self.assertFalse(os.remove.called)

    @mock.patch("logging.Logger.error")
    @mock.patch('os.listdir', return_value=["LiveQuery_ABC123abc_response.json"])
    def test_receive_logs_error_when_unable_to_read_file(self, *mockargs):
        with patch('builtins.open') as mock_open:
            mock_open.side_effect = OSError
            for response in response_receiver.receive():
                raise AssertionError

            self.assertTrue(open.call_count, 1)
            self.assertFalse(os.remove.called)
            self.assertTrue(logging.Logger.error.called)


    def test_remove_response_file_successfully_removes_file(self, *mockargs):
        response_receiver.remove_response_file("/fake/file/path")
        self.assertTrue(os.remove.called)

    @mock.patch("logging.Logger.warning")
    def test_remove_response_file_logs_warning_when_raises_oserror(self, *mockargs):
        with patch('os.remove') as fs:
            fs.side_effect = OSError
            response_receiver.remove_response_file("/fake/file/path")
            self.assertTrue(os.remove.called)
            self.assertTrue(logging.Logger.warning.called)


    @mock.patch('os.listdir', return_value=["LiveQuery_ABC123abc_response.json", "LiveQuery_XYZ987xyz_response.json", "LiveQuery_third_response.json"])
    @mock.patch('builtins.open')
    @mock.patch("logging.Logger.error")
    def test_non_utf8_response_file_throws_error(self, *mockargs):
        open.side_effect = mutating_open_mock


        lst = list(response_receiver.receive())
        self.assertEqual(len(lst), 1)

        response2 = lst[0]
        self.assertEqual(response2[0], os.path.join(RESPONSE_DIR, "LiveQuery_third_response.json"))
        self.assertEqual(response2[1], "LiveQuery")
        self.assertEqual(response2[2], "third")
        self.assertEqual(response2[3], DUMMY_TIMESTAMP)
        self.assertEqual(response2[4], '{"hello": "world"}')
        
        response_dir = "/tmp/sophos-spl/base/mcs/response/"
        self.assertEqual(logging.Logger.error.call_count, 2)
        call_1 = mock.call("Failed to load response json file \"{}\". Error: {}".format(
            os.path.join(response_dir, "LiveQuery_ABC123abc_response.json"),
            "'utf-8' codec can't decode byte 0xa5 in position 3: invalid start byte")
        )
        call_2 = mock.call("Failed to load response json file \"{}\". Error: {}".format(
            os.path.join(response_dir, "LiveQuery_XYZ987xyz_response.json"),
            "'utf-8' codec can't decode byte 0xa5 in position 3: invalid start byte")
        )

        self.assertEqual(logging.Logger.error.call_args_list, [call_1, call_2])
        self.assertEqual(open.call_count, 3)
        self.assertTrue(os.remove.call_count, 3)


if __name__ == '__main__':
    unittest.main()


