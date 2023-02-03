#!/usr/bin/env python

import unittest
from unittest import mock
from unittest.mock import patch, mock_open
import logging
logger = logging.getLogger("TestFileSystemUtils")

import PathManager
import mcsrouter.utils.filesystem_utils as fs_utils


EXAMPLE_FILE = """
ABC=entry1
DEF=entry2
DEF=entry3
"""


class TestFileSystemUtils(unittest.TestCase):

    @mock.patch('os.path.exists', return_value=False)
    def test_read_file_if_exists_returns_none_if_file_non_existent(self, *mockargs):
        self.assertEqual(None, fs_utils.read_file_if_exists('/tmp/sophos-spl'))

    @mock.patch('builtins.open', new_callable=mock_open, read_data="Fedora release 27")
    @mock.patch('os.path.exists', return_value=True)
    def test_read_file_if_exists_returns_contents_if_file_exists(self, *mockargs):
        self.assertEqual("Fedora release 27", fs_utils.read_file_if_exists('/tmp/sophos-spl'))

    @mock.patch('builtins.open', new_callable=mock_open, read_data="")
    @mock.patch('os.path.exists', return_value=True)
    def test_read_file_if_exists_returns_none_if_file_is_empty(self, *mockargs):
        self.assertEqual(None, fs_utils.read_file_if_exists('/tmp/sophos-spl'))

    @mock.patch('os.path.exists', return_value=False)
    def test_return_line_from_file_returns_none_if_file_non_existent(self, *mockargs):
        self.assertEqual(None, fs_utils.return_line_from_file('/tmp/sophos-spl', "ABC"))

    @mock.patch('builtins.open', new_callable=mock_open, read_data="Random unrelated text")
    @mock.patch('os.path.exists', return_value=True)
    def test_return_line_from_file_returns_none_if_file_does_not_contain_given_line(self, *mockargs):
        self.assertEqual(None, fs_utils.return_line_from_file('/tmp/sophos-spl', "ABC"))

    @mock.patch('builtins.open', new_callable=mock_open, read_data=EXAMPLE_FILE)
    @mock.patch('os.path.exists', return_value=True)
    def test_return_line_from_file_returns_first_contents_of_given_line_if_multiple(self, *mockargs):
        self.assertEqual("DEF=entry2", fs_utils.return_line_from_file('/tmp/sophos-spl', "DEF"))

    @mock.patch('builtins.open', new_callable=mock_open, read_data=EXAMPLE_FILE)
    @mock.patch('os.path.exists', return_value=True)
    def test_return_line_from_file_returns_contents_of_given_line(self, *mockargs):
        self.assertEqual("ABC=entry1", fs_utils.return_line_from_file('/tmp/sophos-spl', "ABC"))

    @mock.patch('builtins.open', new_callable=mock_open, read_data="")
    @mock.patch('os.path.exists', return_value=True)
    def test_return_line_from_file_returns_none_if_contents_empty(self, *mockargs):
        self.assertEqual(None, fs_utils.return_line_from_file('/tmp/sophos-spl', "ABC"))


if __name__ == '__main__':
    unittest.main()