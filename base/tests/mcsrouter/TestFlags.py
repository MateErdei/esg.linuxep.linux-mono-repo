#!/usr/bin/env python

import PathManager
import mcsrouter.utils.flags as flags
import mcsrouter.utils.atomic_write

import unittest
import os
import mock
from mock import patch, mock_open
import logging

logger = logging.getLogger("TestResponse")

EXAMPLE_WAREHOUSE_FLAGS = {
    "endpoint.flag1.enabled": "true",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "true",
    "endpoint.flag4.enabled": "force",
    "endpoint.flag5.enabled": "force",
    "endpoint.flag7.enabled": "false",
    "endpoint.flag8.enabled": "true",
    "endpoint.flag10.enabled": "force"
}

EXAMPLE_MCS_FLAGS = {
    "endpoint.flag1.enabled": "true",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "false",
    "endpoint.flag5.enabled": "false",
    "endpoint.flag6.enabled": "true",
    "endpoint.flag7.enabled": "true",
    "endpoint.flag10.enabled": "true"
}

EXAMPLE_COMBINED_FLAGS = {
    "endpoint.flag1.enabled": "true",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "false",
    "endpoint.flag4.enabled": "true",
    "endpoint.flag5.enabled": "true",
    "endpoint.flag6.enabled": "false",
    "endpoint.flag7.enabled": "false",
    "endpoint.flag8.enabled": "false",
    "endpoint.flag10.enabled": "true"
}


EXAMPLE_FULL_WAREHOUSE_FLAGS = {
    "endpoint.flag1.enabled": "true",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "true",
    "endpoint.flag4.enabled": "force",
    "endpoint.flag5.enabled": "force",
    "endpoint.flag6.enabled": "false",
    "endpoint.flag7.enabled": "false",
    "endpoint.flag8.enabled": "true",
    "endpoint.flag10.enabled": "force"
}

EXAMPLE_FULL_MCS_FLAGS = {
    "endpoint.flag1.enabled": "true",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "false",
    "endpoint.flag4.enabled": "false",
    "endpoint.flag5.enabled": "false",
    "endpoint.flag6.enabled": "true",
    "endpoint.flag7.enabled": "true",
    "endpoint.flag8.enabled": "false",
    "endpoint.flag10.enabled": "true"
}

EXAMPLE_FULL_WAREHOUSE_FLAGS_FROM_EMPTY = {
    "endpoint.flag1.enabled": "false",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "false",
    "endpoint.flag5.enabled": "false",
    "endpoint.flag6.enabled": "false",
    "endpoint.flag7.enabled": "false",
    "endpoint.flag10.enabled": "false"
}

EXAMPLE_FULL_MCS_FLAGS_FROM_EMPTY = {
    "endpoint.flag1.enabled": "false",
    "endpoint.flag2.enabled": "false",
    "endpoint.flag3.enabled": "false",
    "endpoint.flag4.enabled": "false",
    "endpoint.flag5.enabled": "false",
    "endpoint.flag7.enabled": "false",
    "endpoint.flag8.enabled": "false",
    "endpoint.flag10.enabled": "false"
}

class TestFlags(unittest.TestCase):
    def test_combine_flags_respects_force(self):
        warehouse_flags = {"endpoint.flag1.enabled": "force"}
        mcs_flags = {"endpoint.flag1.enabled": "false"}
        combined_flags = flags.combine_flags(warehouse_flags, mcs_flags)
        expected_flags = {"endpoint.flag1.enabled": "true"}
        self.assertDictEqual(combined_flags, expected_flags)

    def test_combine_flags_returns_empty_dicts_if_no_flags(self):
        warehouse_flags = {}
        mcs_flags = {}
        combined_flags = flags.combine_flags(warehouse_flags, mcs_flags)
        self.assertDictEqual(combined_flags, {})

    def test_combine_flags_evaluates_a_selection_of_flags(self):
        combined_flags = flags.combine_flags(EXAMPLE_FULL_WAREHOUSE_FLAGS, EXAMPLE_FULL_MCS_FLAGS)
        self.assertDictEqual(combined_flags, EXAMPLE_COMBINED_FLAGS)


    def test_create_comparable_dicts_defaults_missing_keys_to_false(self):
        warehouse_flags, mcs_flags = flags.create_comparable_dicts(EXAMPLE_WAREHOUSE_FLAGS, EXAMPLE_MCS_FLAGS)
        self.assertDictEqual(warehouse_flags, EXAMPLE_FULL_WAREHOUSE_FLAGS)
        self.assertDictEqual(mcs_flags, EXAMPLE_FULL_MCS_FLAGS)

    def test_create_comparable_dicts_handles_first_empty_dict(self):
        warehouse_flags, mcs_flags = flags.create_comparable_dicts(EXAMPLE_WAREHOUSE_FLAGS, {})
        self.assertDictEqual(warehouse_flags, EXAMPLE_WAREHOUSE_FLAGS)
        self.assertDictEqual(mcs_flags, EXAMPLE_FULL_MCS_FLAGS_FROM_EMPTY)

    def test_create_comparable_dicts_handles_second_empty_dict(self):
        warehouse_flags, mcs_flags = flags.create_comparable_dicts({}, EXAMPLE_MCS_FLAGS)
        self.assertDictEqual(warehouse_flags, EXAMPLE_FULL_WAREHOUSE_FLAGS_FROM_EMPTY)
        self.assertDictEqual(mcs_flags, EXAMPLE_MCS_FLAGS)


    @mock.patch('builtins.open', new_callable=mock_open, read_data=b'{"endpoint.flag1.enabled": "false"}')
    def test_read_flags_file_reads_valid_json(self, *mockargs):
        content = flags.read_flags_file("/tmp/mcs-flags.json")
        self.assertDictEqual(content, {"endpoint.flag1.enabled": "false"})

    @mock.patch("logging.Logger.error")
    @mock.patch('builtins.open', new_callable=mock_open, read_data=b'{"endpoint.flag1.enablethisisntvaliddfalse"}')
    def test_read_flags_file_returns_empty_dict_when_json_invalid(self, *mockargs):
        content = flags.read_flags_file("/tmp/mcs-flags.json")
        self.assertDictEqual(content, {})
        self.assertTrue(logging.Logger.error.called)

    @mock.patch("logging.Logger.error")
    @mock.patch('builtins.open', new_callable=mock_open, read_data=b'')
    def test_read_flags_file_returns_empty_dict_when_file_empty(self, *mockargs):
        content = flags.read_flags_file("/tmp/mcs-flags.json")
        self.assertDictEqual(content, {})
        # An empty file with throw a UnicodeDecodeError when the code tries to json.load
        self.assertTrue(logging.Logger.error.called)

    @mock.patch("logging.Logger.error")
    def test_read_flags_file_returns_empty_dict_when_file_does_not_exist(self, *mockargs):
        content = flags.read_flags_file("/tmp/nonexistentfile.json")
        self.assertDictEqual(content, {})
        self.assertTrue(logging.Logger.error.called)

    @mock.patch('mcsrouter.utils.atomic_write.atomic_write')
    @mock.patch('os.chmod')
    @mock.patch("logging.Logger.error")
    def test_write_combined_flags_file_catches_type_error(self, *mockargs):
        flags.write_combined_flags_file({b'asd': "false"})
        self.assertFalse(mcsrouter.utils.atomic_write.atomic_write.called)
        self.assertTrue(logging.Logger.error.called)
        self.assertFalse(os.chmod.called)

    @mock.patch('mcsrouter.utils.atomic_write.atomic_write')
    @mock.patch('os.chmod')
    @mock.patch("logging.Logger.error")
    def test_write_combined_flags_file_writes_file(self, *mockargs):
        flags.write_combined_flags_file({"endpoint.flag1.enabled": "false"})
        self.assertTrue(mcsrouter.utils.atomic_write.atomic_write.called)
        self.assertFalse(logging.Logger.error.called)


if __name__ == '__main__':
    unittest.main()
