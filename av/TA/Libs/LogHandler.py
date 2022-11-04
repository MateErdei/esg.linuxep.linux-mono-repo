
import os
import time
from typing import Optional

from robot.api import logger


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
        assert mark.get_path() == self.__m_log_path
        return mark.get_contents()

    def dump_marked_log(self, mark: LogMark) -> None:
        contents = self.get_contents(mark)
        if contents is None:
            logger.info("File %s does not exist, or failed to read" % str(self.__m_log_path))
        else:
            lines = contents.splitlines()
            lines = [line.decode("UTF-8", errors="backslashreplace") for line in lines]
            logger.info(u"Marked log from %s:\n" % self.__m_log_path+u'\n'.join(lines))
