
import os
import time
from typing import Optional

from robot.api import logger


class LogMark:
    def __init__(self, log_path):
        self.__m_log_path = log_path
        self.__m_stat = os.stat(self.__m_log_path)
        self.__m_mark_time = time.time()
        # Line-count?

    def get_inode(self) -> int:
        return self.__m_stat.st_ino

    def get_size(self) -> int:
        return self.__m_stat.st_size

    def get_path(self) -> str:
        return self.__m_log_path


class LogHandler:
    def __init__(self, log_path: str):
        self.__m_log_path = log_path

    def get_mark(self) -> LogMark:
        return LogMark(self.__m_log_path)

    def get_contents(self, mark: LogMark) -> Optional[bytes]:
        assert mark.get_path() == self.__m_log_path
        try:
            with open(self.__m_log_path, "rb") as f:
                stat = os.fstat(f.fileno())
                if stat.st_ino == mark.get_inode():
                    # File hasn't rotated
                    f.seek(mark.get_size())
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
                    if stat.st_ino == mark.get_inode():
                        # Found old log file
                        f.seek(mark.get_size())
                        return f.read() + contents
                    contents = f.read() + contents
                old_index += 1
        except OSError:
            logger.error("Ran out of log files getting content for "+self.__m_log_path)
            return contents

    def dump_marked_log(self, mark: LogMark) -> None:
        contents = self.get_contents(mark)
        if contents is None:
            logger.info("File %s does not exist, or failed to read" % str(self.__m_log_path))
        else:
            lines = contents.splitlines()
            lines = [line.decode("UTF-8", errors="backslashreplace") for line in lines]
            logger.info(u"Marked log from %s:\n" % self.__m_log_path+u''.join(lines))
