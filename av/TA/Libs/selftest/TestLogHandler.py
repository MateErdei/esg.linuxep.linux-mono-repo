#!/bin/env python3
# Copyright 2023 Sophos Limited. All rights reserved.

import sys
import os

dir = os.path.dirname(os.path.abspath(__file__))
parent = os.path.dirname(dir)

sys.path.append(parent)

import LogHandler

LOG = "/tmp/LogHandlerSelfTest.log"

open(LOG, "w").write("ABC")
open(LOG+".1", "w").write("DEF")
open(LOG+".2", "w").write("GHI")

handler = LogHandler.LogMark(LOG)
assert handler.get_size() == 3

handler = LogHandler.LogMark(LOG, 1)
assert handler.get_size() == 1
assert handler.get_contents() == b"BC", handler.get_contents_unicode() + " is not BC"

statbuf1 = os.stat(LOG+".1")
handler = LogHandler.LogMark(LOG, 1, statbuf1.st_ino)
assert handler.get_size() == 1
assert handler.get_contents() == b"EFABC", handler.get_contents_unicode() + " is not EFABC"

mark = handler.wait_for_log_contains_from_mark("EF", timeout=0.01)
assert mark.get_size() == 1
assert mark.get_inode() == statbuf1.st_ino

statbuf2 = os.stat(LOG+".2")
mark = LogHandler.LogMark(LOG, 1, statbuf2.st_ino)
assert mark.get_size() == 1
assert mark.get_inode() == statbuf2.st_ino
assert mark.get_contents() == b"HIDEFABC", mark.get_contents_unicode() + " is not HIDEFABC"
mark2 = mark.wait_for_log_contains_from_mark("I", timeout=0.01)
assert mark2.get_size() == 2
assert mark2.get_inode() == statbuf2.st_ino
mark3 = mark.wait_for_log_contains_from_mark("E", timeout=0.01)
assert mark3.get_size() == 1
assert mark3.get_inode() == statbuf1.st_ino

