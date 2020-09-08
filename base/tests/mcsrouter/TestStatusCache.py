#!/usr/bin/env python

import unittest
import sys
import time
import hashlib
import mock
import json
#import logging
#logger = logging.getLogger("TestStatusCache")
from mock import patch, mock_open
import PathManager


#try:
import mcsrouter.mcsclient.status_cache

def createCache():
    return mcsrouter.mcsclient.status_cache.StatusCache()

class TestStatusCache(unittest.TestCase):
    def setUp(self):
        self.__m_originalDelay = mcsrouter.mcsclient.status_cache.MAX_STATUS_DELAY

    def tearDown(self):
        mcsrouter.mcsclient.status_cache.MAX_STATUS_DELAY = self.__m_originalDelay

    def test_creation(self):
        cache = createCache()


    def _hash_string(self, string_to_hash):
        hash_object = hashlib.md5(string_to_hash.encode())
        return hash_object.hexdigest()

    def _get_single_record_json_string(self, app_id, status_string):
        status_time_stamp = time.time()
        status_hash = self._hash_string(status_string)
        return r'{"' + app_id + '": {"timestamp": ' + str(status_time_stamp) + ', "status_hash":  "' + status_hash + '"}}'

    def _get_multiple_record_json_string(self, app_id1, status_string1, app_id2, status_string2):
        status_time_stamp = time.time()
        status_hash1 = self._hash_string(status_string1)
        status_hash2 = self._hash_string(status_string2)
        return r'{"' + app_id1 + r'": {"timestamp": ' + str(status_time_stamp) + r', "status_hash":  "' + status_hash1 + r'"}' \
               + r', "' + app_id2 + r'": {"timestamp": ' + str(status_time_stamp) + r', "status_hash":  "' + status_hash2 + r'"}}'

    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testCacheAllowsFirstStatus(self, mock_path, mock_json):
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", "MyStatus")
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertTrue(ret)


    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testCacheAllowsDifferentStatuses(self, mock_path, mock_json):
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", "MyStatus")
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertTrue(ret)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus2")
            self.assertTrue(ret)


    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testCachePreventsDuplicateStatuses(self, mock_path, mock_json):
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", "MyStatus")
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertTrue(ret)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertFalse(ret)


    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testCacheHandlesDifferentAndSameStatusesCorrectly(self, mock_path, mock_json):
        # test to ensure
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", "MyStatus")
        json_dict = json.loads(json_string)

        # 1st status should be added to cache
        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO1", "MyStatus")
            self.assertTrue(ret)

        # 2nd status should be added to cache
        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO2", "MyStatus2")
            self.assertTrue(ret)

        # 3rd status should be in cache
        json_string = self._get_multiple_record_json_string("FOO1", "MyStatus", "FOO2", "MyStatus2")
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO1", "MyStatus")
            self.assertFalse(ret)


    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testCacheAllowsStatusAfterDelay(self, mock_path, mock_json):
        mcsrouter.mcsclient.status_cache.MAX_STATUS_DELAY = 1
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", "MyStatus")
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertTrue(ret)

        time.sleep(2)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO", "MyStatus")
            self.assertTrue(ret)

    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testTimestampRemoved(self, mock_path, mock_json):
        # Check that the time stamp is correctly removed to verify the status messages can be correctly compared.
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", '  MyStatus')
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", ' timestamp="2017-09-14T15:43:10.74112Z" MyStatus')
            self.assertTrue(ret)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO", ' timestamp="2017-09-14T19:43:10.74112Z" MyStatus')
            self.assertFalse(ret)

    @mock.patch('mcsrouter.mcsclient.status_cache.json')
    @mock.patch('mcsrouter.mcsclient.status_cache.os.path')
    def testEscapedTimestampRemoved(self, mock_path, mock_json):
        # Check that the time stamp is correctly removed to verify the status messages can be correctly compared.
        cache = createCache()

        json_string = self._get_single_record_json_string("FOO", '  MyStatus')
        json_dict = json.loads(json_string)

        mock_path.isfile.return_value = False
        mocked_open_write_function = mock.mock_open()
        with mock.patch("builtins.open", mocked_open_write_function):
            ret = cache.has_status_changed_and_record("FOO", ' timestamp=&quot;2017-09-14T15:43:10.74112&quot;')
            self.assertTrue(ret)

        mock_path.isfile.return_value = True
        mock_json.load.return_value = json_dict
        mocked_open_read_function = mock.mock_open(read_data=json_string)
        with mock.patch("builtins.open", mocked_open_read_function):
            ret = cache.has_status_changed_and_record("FOO", ' timestamp=&quot;2017-09-14T19:43:10.74112&quot; MyStatus')
            self.assertFalse(ret)

    @patch('os.path.isfile')
    def testEmptyStatusCacheOnDiskDoesNotCrash(self, mock_isfile):
        cache = createCache()
        mock_isfile.return_value = True
        with patch("builtins.open", mock_open(read_data="")) as mock_file:
            with mock.patch("os.chmod", return_value=True):
                ret = cache.has_status_changed_and_record("FOO", ' timestamp=&quot;2017-09-14T15:43:10.74112&quot;')
                self.assertTrue(ret)

    @patch('os.path.isfile')
    def testInvalidStatusCacheOnDiskDoesNotCrash(self, mock_isfile):
        cache = createCache()
        mock_isfile.return_value = True
        with patch("builtins.open", mock_open(read_data="not json")) as mock_file:
            with mock.patch("os.chmod", return_value=True):
                ret = cache.has_status_changed_and_record("FOO", ' timestamp=&quot;2017-09-14T15:43:10.74112&quot;')
                self.assertTrue(ret)


# except ImportError:
#     #logger.error("Bad sys.path: %s",str(sys.path))
#     class TestStatusCache(unittest.TestCase):
#         pass
# except Exception as e:
#     print("failed because: {}".format(e))

if __name__ == '__main__':
    #if "-q" in sys.argv:
    #    logging.disable(logging.CRITICAL)
    unittest.main()


