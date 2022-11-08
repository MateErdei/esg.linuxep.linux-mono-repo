#!/usr/bin/env python3

import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

try:
    from . import LogHandler
except ImportError:
    import LogHandler

ensure_binary = LogHandler.ensure_binary


def ensure_unicode(s) -> str:
    if isinstance(s, bytes):
        return s.decode("UTF-8", errors="replace")
    return s


def get_content_lines(path, mark) -> list:
    if isinstance(mark, LogHandler.LogMark):
        mark.assert_is_good(path)
        contents = mark.get_contents()
        if contents is None:
            return []
        return contents.splitlines(keepends=True)
    assert isinstance(mark, int)
    contents = open(path, encoding="utf-8", errors="backslashreplace").readlines()
    return contents[mark:]


def get_sophos_threat_detector_pid_or_none():
    """
    Could write this using subprocess and pidof, or using some 3rd party lib.
    :return:
    """
    for pid in os.listdir("/proc"):
        p = "/proc/%s/exe" % pid
        try:
            dest = os.readlink(p)
        except EnvironmentError:
            continue

        if dest.startswith("/opt/sophos-spl/plugins/av/sbin/sophos_threat_detector"):
            return int(pid)
    return None


class SophosThreatDetectorException(Exception):
    pass


class SophosThreatDetectorRestartedException(SophosThreatDetectorException):
    pass


class SophosThreatDetectorFailedToLogException(SophosThreatDetectorException):
    pass


def get_sophos_threat_detector_pid():
    """
    Could write this using subprocess and pidof, or using some 3rd party lib.
    :return:
    """
    pid = get_sophos_threat_detector_pid_or_none()
    if pid is not None:
        return pid
    raise SophosThreatDetectorException("sophos_threat_detector not running")


def normalize_mark(mark):
    if isinstance(mark, LogHandler.LogMark):
        return mark

    try:
        return int(mark)
    except TypeError:
        logger.error("Unable to convert mark ("+str(type(mark))+") to int")
    return mark


class SophosThreatDetector(object):
    def __init__(self):
        self.__m_ides_for_uninstall = []

    @staticmethod
    def Wait_Until_Sophos_Threat_Detector_Logs_On_Off_Or_Restarts(original_pid, mark, texts, timeout=15, interval=3):
        """
        Waits for sophos_threat_detector to log one of <texts> or to restart, or to stop
        :param original_pid:
        :param mark:
        :param texts:
        :param timeout:
        :param interval:
        :return: text that was found, or None if sophos_threat_detector stopped, False if no texts found
        """
        original_pid = int(original_pid)
        mark = normalize_mark(mark)
        timeout = float(timeout)
        interval = float(interval)

        texts = [ ensure_unicode(t) for t in texts ]

        log_file_path = BuiltIn().get_variable_value("${THREAT_DETECTOR_LOG_PATH}")
        start = time.time()
        end = start + timeout
        contents = []

        while time.time() < end:
            logger.debug("Checking log after %f seconds" % (time.time() - start))
            contents = get_content_lines(log_file_path, mark)
            for line in contents:
                line = ensure_unicode(line)
                for text in texts:
                    if text in line:
                        logger.info("Found line %s in marked log contents" % text)
                        return text

            pid = get_sophos_threat_detector_pid_or_none()
            if pid != original_pid:
                return None

            time.sleep(interval)

        contents = "".join(contents)
        logger.info("Failed to find any of texts in contents: %s" % contents)
        return False

    @staticmethod
    def Wait_Until_Sophos_Threat_Detector_Logs_Or_Restarts(original_pid, mark, text, timeout=15, interval=3):
        """
        Throw exception if sophos_threat_detector restarts

        :param interval:
        :param original_pid:
        :param mark:
        :param text:
        :param timeout:
        :return:
        """
        original_pid = int(original_pid)
        mark = normalize_mark(mark)
        timeout = float(timeout)
        interval = float(interval)
        text = ensure_unicode(text)

        log_file_path = BuiltIn().get_variable_value("${THREAT_DETECTOR_LOG_PATH}")
        start = time.time()
        end = start + timeout
        contents = []

        while time.time() < end:
            logger.debug("Checking log after %f seconds" % (time.time() - start))
            contents = get_content_lines(log_file_path, mark)
            for line in contents:
                line = ensure_unicode(line)
                if text in line:
                    logger.info("Found line %s in marked log contents" % text)
                    return True

            pid = get_sophos_threat_detector_pid()
            if pid != original_pid:
                raise SophosThreatDetectorRestartedException(
                    "sophos_threat_detector has been restarted! original_pid=%d, current pid=%d" % (original_pid, pid))

            time.sleep(interval)

        contents = "".join(contents)
        logger.info("Contents: %s" % contents)
        raise SophosThreatDetectorFailedToLogException("Failed to find text %s in log" % text)
