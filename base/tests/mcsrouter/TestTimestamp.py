#!/usr/bin/env python



import os
import unittest
import sys
import json

import logging
logger = logging.getLogger("TestTimestamp")

import PathManager

import mcsrouter.utils.timestamp

class TestTimestamp(unittest.TestCase):
    def testFormatting(self):
        res = mcsrouter.utils.timestamp.timestamp(1000)
        self.assertEqual(res,"1970-01-01T00:16:40.0Z")

    def testMultipleExecutionsDontGiveSameAnswer(self):
        res1 = mcsrouter.utils.timestamp.timestamp()
        res2 = mcsrouter.utils.timestamp.timestamp()
        self.assertNotEqual(res1,res2)

if __name__ == '__main__':
    unittest.main()
