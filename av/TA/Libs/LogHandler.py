#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import os
import re
import six
import time
from typing import Optional

from robot.api import logger


def ensure_binary(s, encoding="UTF-8"):
    if isinstance(s, six.text_type):
        return s.encode(encoding)
    return s


class LogMark:
    def __init__(self, log_path):
        self.__m_log_path = log_path
        try:
            self.__m_stat = os.stat(self.__m_log_path)
            self.__m_inode = self.__m_stat.st_ino
        except OSError:
            self.__m_stat = None
            self.__m_inode = None

        self.__m_mark_time = time.time()
        # Line-count?

    def __str__(self):
        mark_time = time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(self.__m_mark_time))
        if self.__m_stat is None:
            return "Missing file at %s" % mark_time
        return "%d bytes at %s" % (self.__m_stat.st_size, mark_time)

    def get_inode(self) -> int:
        return self.__m_inode

    def check_inode(self, inode=None) -> bool:
        """
        Check if the log files inode matches the inode saved at mark construction time
        :return:
        """
        if inode is None:
            # examine the file directly
            try:
                stat = os.stat(self.__m_log_path)
                inode = stat.st_ino
            except OSError:
                return True

        if self.__m_inode is None:
            self.__m_inode = inode

        return inode == self.__m_inode

    def get_size(self) -> int:
        if self.__m_stat is not None:
            return self.__m_stat.st_size
        # log file didn't exist when Mark created, so entire file is valid
        return 0

    def get_path(self) -> str:
        return self.__m_log_path

    def get_contents(self) -> Optional[bytes]:
        try:
            with open(self.__m_log_path, "rb") as f:
                stat = os.fstat(f.fileno())
                if self.check_inode(stat.st_ino):
                    # File hasn't rotated
                    f.seek(self.get_size())
                    return f.read()
                contents = f.read()
        except OSError:
            return None
        # log file has rotated
        old_index = 1
        try:
            while True:
                with open(self.__m_log_path+".%d" % old_index, "rb") as f:
                    stat = os.fstat(f.fileno())
                    if stat.st_ino == self.get_inode():
                        # Found old log file
                        f.seek(self.get_size())
                        return f.read() + contents
                    contents = f.read() + contents
                old_index += 1
        except OSError:
            logger.error("Ran out of log files getting content for "+self.__m_log_path)
            return contents

    def assert_is_good(self, log_path: str):
        assert self.get_path() == log_path

    def dump_marked_log(self) -> None:
        contents = self.get_contents()
        if contents is None:
            logger.info("File %s does not exist, or failed to read" % str(self.__m_log_path))
        else:
            lines = contents.splitlines()
            lines = [line.decode("UTF-8", errors="backslashreplace") for line in lines]
            logger.info(u"Marked log from %s:\n" % self.__m_log_path+u'\n'.join(lines))

    def wait_for_log_contains_from_mark(self, expected, timeout) -> None:
        expected = ensure_binary(expected, "UTF-8")
        start = time.time()
        old_contents = ""
        while time.time() < start + timeout:
            contents = self.get_contents()
            if contents is not None:
                if len(contents) > len(old_contents):
                    logger.debug(contents[:len(old_contents)])

                if expected in contents:
                    return

                old_contents = contents

            time.sleep(0.5)

        logger.error("Failed to find %s in %s after %s" % (expected, self.get_path(), self))
        self.dump_marked_log()
        raise AssertionError("Failed to find %s in %s" % (expected, self.get_path()))


class LogHandler:
    def __init__(self, log_path: str):
        self.__m_log_path = log_path

    def get_mark(self) -> LogMark:
        return LogMark(self.__m_log_path)

    def assert_mark_is_good(self, mark: LogMark):
        assert isinstance(mark, LogMark)
        mark.assert_is_good(self.__m_log_path)

    def get_contents(self, mark: LogMark) -> Optional[bytes]:
        assert isinstance(mark, LogMark)
        mark.assert_is_good(self.__m_log_path)
        return mark.get_contents()

    def dump_marked_log(self, mark: LogMark) -> None:
        assert isinstance(mark, LogMark)
        mark.assert_is_good(self.__m_log_path)
        return mark.dump_marked_log()

    def wait_for_log_contains_from_mark(self, mark: LogMark, expected, timeout) -> None:
        assert isinstance(mark, LogMark)
        mark.assert_is_good(self.__m_log_path)
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def __generate_log_file_names(self):
        yield self.__m_log_path
        for n in range(1, 10):
            yield "%s.%d" % (self.__m_log_path, n)

    def get_content_since_last_start(self) -> list:
        """
        Get the log file contents since the last start,
        assuming the first field in a log line is time(ms) since the process started
        assumes 999999999 is greater than any process will live for
        :return:
        """
        results = []
        proc_age = 999999999
        LINE_RE = re.compile(rb"^(\d+).*")

        for file_path in self.__generate_log_file_names():
            lines = open(file_path, "rb").readlines()
            lines = reversed(lines)  # read the newest first
            for line in lines:
                mo = LINE_RE.match(line)
                if mo:
                    age = int(mo.group(1))
                    if age > proc_age:
                        results.reverse()
                        return results
                    proc_age = age
                results.append(line)

        logger.error("Log file %s has completely rolled over since process last started" % self.__m_log_path)
        results.reverse()
        return results

    def Wait_For_AV_Log_contains_after_last_restart(self, expected, timeout: int) -> None:
        """
        Need to look for the restart in the log, and check the log after that.
        A restart means the first digit resetting to 0
        :param expected:
        :param timeout:
        :return:
        """
        expected = ensure_binary(expected, "UTF-8")
        start = time.time()
        content_lines = []
        while time.time() < start + timeout:
            content_lines = self.get_content_since_last_start()
            for line in content_lines:
                if expected in line:
                    return

            time.sleep(1)

        content_lines = [line.decode("UTF-8", errors="backslashreplace") for line in content_lines]
        logger.info("AV Log since last restart:" + u"".join(content_lines))
        raise AssertionError("'%s' not found in %s after %d seconds" % (expected, self.__m_log_path, timeout))
