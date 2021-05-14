#!/usr/bin/env python3

import os
import time

from robot.libraries.BuiltIn import BuiltIn
from robot.api import logger

def get_content_lines(path, mark):
    contents = open(path).readlines()
    return contents[mark:]

def get_sophos_threat_detector_pid():
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
    raise Exception("sophos_threat_detector not running")


class SophosThreatDetector(object):
    def __init__(self):
        self.__m_ides_for_uninstall = []

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
        mark = int(mark)
        timeout = float(timeout)
        interval = float(interval)

        log_file_path = BuiltIn().get_variable_value("${THREAT_DETECTOR_LOG_PATH}")
        start = time.time()
        end = start + timeout
        contents = []

        while time.time() < end:
            logger.debug("Checking log after %f seconds" % (time.time() - start))
            contents = get_content_lines(log_file_path, mark)
            for line in contents:
                if text in line:
                    logger.info("Found line %s in marked log contents" % text)
                    return True

            pid = get_sophos_threat_detector_pid()
            if pid != original_pid:
                raise Exception("sophos_threat_detector has been restarted! original_pid=%d, current pid=%d" % (original_pid, pid))

            time.sleep(interval)

        contents = "\n".join(contents)
        logger.error("Contents: %s" % contents)
        raise Exception("Failed to find text %s in log" % text)

    def register_ide_for_uninstall(self, idename):
        self.__m_ides_for_uninstall.append(idename)

    def deregister_ide_for_uninstall(self, idename):
        try:
            self.__m_ides_for_uninstall.remove(idename)
        except ValueError:
            logger.error("Attempted to remove IDE not on uninstall list")

    def cleanup_ides(self):
        """
    # We don't know how the cleanup was registered
    Deregister Optional Cleanup   Uninstall IDE  ${ide_name}  ${ide_update_func}
    Deregister Optional Cleanup   Uninstall IDE  ${ide_name}
    Remove IDE From Install Set  ${ide_name}
    Run Keyword  ${ide_update_func}
    Check IDE Absent From Installation  ${ide_name}
    """
        builtin = BuiltIn()
        for idename in self.__m_ides_for_uninstall:
            builtin.run_keyword("Remove IDE From Install Set", idename)

        builtin.run_keyword("Run installer from install set")

        for idename in self.__m_ides_for_uninstall:
            builtin.run_keyword("Check IDE Absent From Installation", idename)

        self.__m_ides_for_uninstall = []