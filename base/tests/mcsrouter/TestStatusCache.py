#!/usr/bin/env python



import unittest
import sys
import time

#import logging
#logger = logging.getLogger("TestStatusCache")

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

    def testCacheAllowsFirstStatus(self):
        cache = createCache()
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertTrue(ret)


    def testCacheAllowsDifferentStatuses(self):
        cache = createCache()
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertTrue(ret)
        ret = cache.has_status_changed_and_record("FOO","MyStatus2")
        self.assertTrue(ret)

    def testCachePreventsDuplicateStatuses(self):
        cache = createCache()
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertTrue(ret)
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertFalse(ret)

    def testCacheAllowsStatusAfterDelay(self):
        mcsrouter.mcsclient.status_cache.MAX_STATUS_DELAY = 1
        cache = createCache()
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertTrue(ret)
        time.sleep(2)
        ret = cache.has_status_changed_and_record("FOO","MyStatus")
        self.assertTrue(ret)

    def testTimestampRemoved(self):
        cache = createCache()
        ret = cache.has_status_changed_and_record("FOO",' timestamp="2017-09-14T15:43:10.74112Z"')
        self.assertTrue(ret)
        ret = cache.has_status_changed_and_record("FOO",' timestamp="2017-09-14T19:43:10.74112Z"')
        self.assertFalse(ret)

    def testEscapedTimestampRemoved(self):
        cache = createCache()
        ret = cache.has_status_changed_and_record("NTP",' timestamp=&quot;2017-09-14T15:43:10.74112&quot;')
        self.assertTrue(ret)
        ret = cache.has_status_changed_and_record("NTP",' timestamp=&quot;2017-09-14T19:43:10.74112&quot;')
        self.assertFalse(ret)

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


