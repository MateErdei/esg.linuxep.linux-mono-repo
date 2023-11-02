#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright 2022-2023 Sophos Limited. All rights reserved.

import os
import re
import six
import time
from typing import Optional

try:
    from robot.api import logger
except ImportError:
    import logging
    logger = logging.getLogger("LogHandler")


def ensure_binary(s, encoding="UTF-8"):
    if isinstance(s, list):
        return [ ensure_binary(x) for x in s ]
    elif isinstance(s, six.text_type):
        return s.encode(encoding)
    return s


def ensure_unicode(s, encoding="UTF-8"):
    if isinstance(s, list):
        return [ ensure_unicode(x) for x in s ]
    elif isinstance(s, six.binary_type):
        return s.decode(encoding, errors="backslashreplace")
    return s


class LogMark:
    def __init__(self, log_path, position=-1, inode=None):
        self.__m_log_path = log_path
        self.__m_override_position = position
        if inode is not None:
            self.__m_stat = None
            self.__m_inode = inode
            assert position != -1
        else:
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
        if self.__m_override_position >= 0:
            return "%d bytes in %s in inode %d" %(self.get_size(), mark_time, self.__m_inode)
        if self.__m_inode is None:
            return "Missing file at %s" % mark_time
        return "%d bytes at %s" % (self.get_size(), mark_time)

    def get_inode(self) -> int:
        return self.__m_inode

    def check_inode(self, inode=None) -> bool:
        """
        Check if the log files inode matches the inode saved at mark construction time
        :return: True if the inode matches our saved inode
        """
        if self.__m_inode is None:
            # If the file didn't exist when mark was made, then all files are valid
            return False

        if inode is None:
            # examine the file directly
            try:
                stat = os.stat(self.__m_log_path)
                inode = stat.st_ino
            except OSError:
                return True

        return inode == self.__m_inode

    def get_size(self) -> int:
        if self.__m_override_position >= 0:
            return self.__m_override_position

        if self.__m_stat is not None:
            return self.__m_stat.st_size
        # log file didn't exist when Mark created, so entire file is valid
        return 0

    def get_path(self) -> str:
        return self.__m_log_path

    def get_current_contents(self) -> Optional[bytes]:
        """
        Get the contents of the current log file if present
        :return:
        """
        try:
            with open(self.__m_log_path, "rb") as f:
                return f.read()
        except OSError:
            return None

    def __generate_log_paths(self):
        yield self.__m_log_path
        old_index = 1
        while True:
            yield self.__m_log_path + ".%d" % old_index
            old_index += 1

    def __generate_contents(self):
        for log_path in self.__generate_log_paths():
            try:
                with open(log_path, "rb") as f:
                    stat = os.fstat(f.fileno())
                    if stat.st_ino == self.get_inode():
                        # Found the originally marked file, so only get the new content and finish
                        f.seek(self.get_size())
                        yield stat.st_ino, f.read()
                        return

                    # Not the marked file, so return the entire file
                    yield stat.st_ino, f.read()
            except OSError:
                if self.__m_inode is not None:
                    # no inode means nothing counts as old
                    logger.error("Ran out of log files getting content for " + self.__m_log_path)
                return

    def __generate_full_contents(self):
        for log_path in self.__generate_log_paths():
            try:
                with open(log_path, "rb") as f:
                    stat = os.fstat(f.fileno())
                    yield stat.st_ino, f.read()
            except OSError:
                return

    def get_contents(self) -> Optional[bytes]:
        contents = list(self.__generate_contents())
        contents.reverse()
        if len(contents) == 0:
            return None
        return b"".join((content for _, content in contents))

    def get_contents_unicode(self) -> Optional[str]:
        contents = self.get_contents()
        if contents is None:
            return contents
        return ensure_unicode(contents)

    def generate_reversed_lines(self):
        contents = self.get_contents()
        if contents is None:
            return []
        else:
            lines = contents.splitlines(keepends=True)
            lines.reverse()
            for line in lines:
                yield line

    def assert_is_good(self, log_path: str):
        assert self.get_path() == log_path, "mark is for wrong file"

    def assert_paths_match(self, log_path: str):
        assert self.get_path() == log_path, "mark is for wrong file"

    def dump_marked_log(self) -> None:
        contents = self.get_contents()
        if contents is None:
            logger.info("File %s does not exist, or failed to read" % str(self.__m_log_path))
        else:
            lines = contents.splitlines()
            lines = [line.decode("UTF-8", errors="backslashreplace") for line in lines]
            logger.info(u"Marked log from %s:\n" % self.__m_log_path + u'\n'.join(lines))

    def __check_for_str_in_contents(self, expected, contents):
        if isinstance(expected, list):
            for s in expected:
                if s in contents:
                    return True
            return False
        else:
            return expected in contents

    def __find_str_in_contents(self, expected, contents, start=0):
        """
        :param expected: str or list to search for: returns position of first entry in list found
        :param contents:
        :return: -1 if expected not in contents
        """
        if contents is None:
            return -1
        if isinstance(expected, list):
            positions = []
            for s in expected:
                pos = self.__find_str_in_contents(s, contents, start)
                if pos >= 0:
                    positions.append(pos)
            if len(positions) > 0:
                return min(positions)
            return -1
        else:
            return contents.find(expected, start)

    def __internal_wait_for_possible_log_contains_from_mark(self, expected, timeout: float) -> Optional['LogMark']:
        expected = ensure_binary(expected, "UTF-8")
        start = time.time()
        sleep_time = timeout / 60  # Check by default 60 times during the timeout
        sleep_time = max(0.01, sleep_time)  # Delay a minimum of 10ms between checks
        sleep_time = min(10.0, sleep_time)    # Delay a maximum of 10 seconds between checks
        logger.debug("Sleeping %f seconds between checks" % sleep_time)
        old_contents = ""
        while time.time() < start + timeout:
            contents = self.get_contents()
            if contents is not None and old_contents != contents:
                if len(contents) > len(old_contents):
                    logger.debug(contents[len(old_contents):])

                pos = self.__find_str_in_contents(expected, contents)
                if pos >= 0:
                    # We found a match, now we need to return a new LogMark for the position of the match
                    stat = os.stat(self.__m_log_path)
                    if self.__m_inode == stat.st_ino:
                        # mark is within the current file, so pos must be as well
                        absolute_pos = pos + self.get_size()
                        return LogMark(self.__m_log_path, absolute_pos, stat.st_ino)
                    else:
                        # mark is is a previous log file
                        # pos might be in a previous file
                        for inode, contents in self.__generate_full_contents():
                            absolute_pos = self.__find_str_in_contents(expected, contents)  # see if we have a match
                            if absolute_pos > -1:
                                # A match in this log file, so return if
                                return LogMark(self.__m_log_path, absolute_pos, inode)

                        # No good match - log files must be rotating too fast - so use start of current file
                        logger.error("Failed to find matching mark for found contents - log files rotating too fast!")
                        return LogMark(self.__m_log_path, 0)

                old_contents = contents
            time.sleep(sleep_time)
        # Failed to find a match in time
        return None

    def wait_for_possible_log_contains_from_mark(self, expected, timeout: float) -> 'LogMark':
        """
        Wait for the log to contain the text specified

        :param expected: List of strings or String that we are expecting
        :param timeout: Log info level if we spend too long
        :return: New LogMark for the position we found the match
        """
        mark = self.__internal_wait_for_possible_log_contains_from_mark(expected, timeout)
        if mark:
            return mark

        logger.info("Failed to find %s in %s after %s" % (expected, self.get_path(), self))
        return self

    def wait_for_log_contains_from_mark(self, expected, timeout: float) -> 'LogMark':
        """
        Wait for the log to contain the text specified

        :param expected: List of strings or String that we are expecting
        :param timeout: Fail the test if we spend too long waiting
        :return: New LogMark for the position we found the match
        """
        mark = self.__internal_wait_for_possible_log_contains_from_mark(expected, timeout)
        if mark:
            return mark

        expected = ensure_binary(expected, "UTF-8")
        logger.error("Failed to find %s in %s after %s" % (expected, self.get_path(), self))
        self.dump_marked_log()
        raise AssertionError("Failed to find %s in %s" % (expected, self.get_path()))

    def wait_for_log_contains_one_of_from_mark(self, expectedList, timeout) -> None:
        expectedList = ensure_binary(expectedList, "UTF-8")
        start = time.time()
        old_contents = ""
        while time.time() < start + timeout:
            contents = self.get_contents()
            if contents is not None:
                if len(contents) > len(old_contents):
                    logger.debug(contents[:len(old_contents)])

                for expected in expectedList:
                    if self.__check_for_str_in_contents(expected, contents):
                        return

                old_contents = contents

            time.sleep(0.5)

        logger.error("Failed to find %s in %s after %s" % (expected, self.get_path(), self))
        self.dump_marked_log()
        raise AssertionError("Failed to find %s in %s" % (expected, self.get_path()))

    def wait_for_log_contains_n_times_from_mark(self, expected, times, timeout) -> None:
        assert times >= 1
        expected = ensure_binary(expected, "UTF-8")
        start = time.time()
        sleep_time = timeout / 60  # Check by default 60 times during the timeout
        sleep_time = max(0.01, sleep_time)  # Delay a minimum of 10ms between checks
        sleep_time = min(10.0, sleep_time)    # Delay a maximum of 10 seconds between checks
        logger.debug("Sleeping %f seconds between checks" % sleep_time)
        old_contents = ""
        while time.time() < start + timeout:
            contents = self.get_contents()
            if contents is not None:
                if len(contents) > len(old_contents):
                    logger.debug(contents[:len(old_contents)])
                    old_contents = contents

                pos = 0
                for i in range(times):
                    pos = self.__find_str_in_contents(expected, contents, pos)

                if pos >= 0:
                    return pos

            time.sleep(sleep_time)

        logger.error("Failed to find %s %d times in %s after %s" % (expected, times, self.get_path(), self))
        self.dump_marked_log()
        raise AssertionError("Failed to find %s in %s" % (expected, self.get_path()))

    def find_last_match_after_mark(self, line_to_search_for):
        contents = self.get_contents()
        pos = contents.rfind(ensure_binary(line_to_search_for))
        if pos < 0:
            raise AssertionError("Failed to find %s in %s" % (line_to_search_for, self.get_path()))
        absolute_pos = pos + self.get_size()
        stat = os.stat(self.__m_log_path)
        if stat.st_size < absolute_pos:
            absolute_pos = stat.st_size
        return LogMark(self.__m_log_path, absolute_pos)


class LogHandler:
    def __init__(self, log_path: str):
        self.__m_log_path = log_path

    def get_mark(self) -> LogMark:
        return LogMark(self.__m_log_path)

    def get_mark_at_start_of_current_file(self) -> LogMark:
        return LogMark(self.__m_log_path, 0)

    def assert_mark_is_good(self, mark: LogMark):
        assert isinstance(mark, LogMark)
        mark.assert_paths_match(self.__m_log_path)

    def get_contents(self, mark: LogMark) -> Optional[bytes]:
        assert isinstance(mark, LogMark), "mark is not an instance of LogMark"
        mark.assert_paths_match(self.__m_log_path)
        return mark.get_contents()

    def dump_marked_log(self, mark: LogMark) -> None:
        assert isinstance(mark, LogMark)
        mark.assert_paths_match(self.__m_log_path)
        return mark.dump_marked_log()

    def wait_for_log_contains_from_mark(self, mark: LogMark, expected, timeout) -> None:
        assert isinstance(mark, LogMark)
        mark.assert_paths_match(self.__m_log_path)
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    @staticmethod
    def __readlines(file_path):
        try:
            return open(file_path, "rb").readlines()
        except OSError:
            return []

    @staticmethod
    def __read_content(file_path):
        """
        Reads a file, or returns "" if the file doesn't exist or can't be read.

        :param file_path: File path to read contents of.
        :return: file contents or returns "" if the file doesn't exist or can't be read.
        """
        try:
            return open(file_path, "rb").read()
        except OSError:
            return b""

    def __generate_log_file_names(self):
        yield self.__m_log_path
        for n in range(1, 10):
            yield "%s.%d" % (self.__m_log_path, n)

    def __generate_reversed_lines(self, mark=None):
        if mark is None:
            for file_path in self.__generate_log_file_names():
                lines = self.__readlines(file_path)
                lines = reversed(lines)  # read the newest first
                for line in lines:
                    yield line
        else:
            mark.assert_paths_match(self.__m_log_path)
            for line in mark.generate_reversed_lines():
                yield line

    def get_content_since_last_start(self, mark=None) -> list:
        """
        Get the log file contents since the last start, assuming the process
        starts by logging: "Logger .* configured for level:"
        :return: list<bytes> lines from current run of process
        """
        results = []
        START_RE = re.compile(rb".*<> Logger .* configured for level: ")
        for line in self.__generate_reversed_lines(mark):  # read the newest first
            mo = START_RE.match(line)
            results.append(line)
            if mo:
                logger.info("Found start line: "+line.decode("UTF-8"))
                results.reverse()
                return results

        # Failed to find start: check age of the oldest line
        results.reverse()

        # No error if we have no results
        if len(results) > 0:
            LINE_RE = re.compile(rb"^(\d+)\s+\[.*")
            age = None
            for line in results:
                mo = LINE_RE.match(line)
                if mo:
                    age = int(mo.group(1))
                    break
            if age is not None:
                logger.error(
                    "Log file %s has completely rolled over since process last started (%d lines) (earliest log: %d):" %
                    (self.__m_log_path, len(results), age))
                if len(results) < 50:
                    for line in results:
                        logger.info(line.decode("UTF-8", errors="replace"))

        return results

    def Wait_For_Log_contains_after_last_restart(self, expected, timeout: int, mark=None) -> None:
        """
        Need to look for the restart in the log, and check the log after that.
        A restart means the first digit resetting to 0
        :param mark: Optional Mark - only check log after mark
        :param expected: String expected in log
        :param timeout: Amount of time to wait for expected to appear
        :return: None
        """
        expected = ensure_binary(expected, "UTF-8")
        start = time.time()
        content_lines = []
        while time.time() < start + timeout:
            content_lines = self.get_content_since_last_start(mark)
            for line in content_lines:
                if expected in line:
                    return

            time.sleep(1)

        content_lines = [line.decode("UTF-8", errors="backslashreplace") for line in content_lines]
        logger.info("%s since last restart:" % self.__m_log_path + u"".join(content_lines))
        raise AssertionError("'%s' not found in %s after %d seconds" % (expected, self.__m_log_path, timeout))

    def Wait_For_Entire_log_contains(self, expected, timeout: int) -> None:
        """
        Waits for expected to appear in the entire text of the logs
        :param expected: str or bytes we are looking for in the log file
        :param timeout: timeout in seconds
        :return: None, throws AssertionError on failure
        """
        expected_bytes = ensure_binary(expected, "UTF-8")
        start = time.time()
        while time.time() < start + timeout:
            for file_path in self.__generate_log_file_names():
                if expected_bytes in self.__read_content(file_path):
                    return
            time.sleep(1)

        logger.error("Failed to find %s in any log files for %s" % (expected, self.__m_log_path))
        raise AssertionError("Failed to find %s in any log files for %s" % (expected, self.__m_log_path))

    def check_log_contains_expected_after_unexpected(self, expected, unexpected):
        log_path = self.__m_log_path
        expected = ensure_binary(expected)
        unexpected = ensure_binary(unexpected)
        contents = b"".join(self.get_content_since_last_start())
        contentsStr = ensure_unicode(contents)
        expected_find = contents.rfind(expected)
        if expected_find >= 0:
            remainder = contents[expected_find:]
            if unexpected not in remainder:
                return True
            logger.info("Searched contents of %s: %s" % (log_path, contentsStr))
            raise AssertionError("Found unexpected %s after expected %s in %s" % (unexpected, expected, log_path))
        logger.info("Searched contents of %s: %s" % (log_path, contentsStr))

        if unexpected in contents:
            raise AssertionError("Found unexpected %s but not expected %s in %s" % (unexpected, expected, log_path))

        raise AssertionError("Failed to find expected %s in %s" % (expected, log_path))
