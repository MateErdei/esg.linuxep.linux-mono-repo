import subprocess
import os
import time
import grp
import pwd
import glob

import logging
logger = logging.getLogger("AVPlugin")
logger.setLevel(logging.DEBUG)


def _sophos_spl_path():
    return os.environ['SOPHOS_INSTALL']


PLUGIN_NAME = "av"


def _av_plugin_dir():
    return os.path.join(_sophos_spl_path(), 'plugins', PLUGIN_NAME)


def _av_exec_path():
    return os.path.join(_av_plugin_dir(), 'sbin', PLUGIN_NAME)


def _log_path():
    return os.path.join(_av_plugin_dir(), 'log')


def _plugin_log_path():
    return os.path.join(_log_path(), PLUGIN_NAME+'.log')


def _file_content(path):
    with open(path, 'r') as f:
        return f.read()


class AVPlugin(object):
    def __init__(self):
        self._proc = None
        self._failed = False

    def set_failed(self):
        self._failed = True

    def plugin_path(self):
        return _av_plugin_dir()

    def log_path(self):
        return _log_path()

    def _ensure_sophos_required_unix_user_and_group_exists(self):
        try:
            grp.getgrnam('sophos-spl-group')
            pwd.getpwnam('sophos-spl-user')
        except KeyError as ex:
            raise AssertionError("Sophos spl group, or user not present: {}".format(ex))

    def prepare_for_test(self):
        self.stop_av()
        self._ensure_sophos_required_unix_user_and_group_exists()

    def start_av(self):
        logger.debug("Start start_av")
        self.prepare_for_test()
        self._proc = subprocess.Popen([_av_exec_path()])
        logger.debug("Finish start_av")

    def stop_av(self):
        """
        Stop Plugin and allow graceful termination
        """
        logger.debug("Start stop_av")
        if self._proc:
            self._proc.terminate()
            try:
                self._proc.wait(15)
            except subprocess.TimeoutExpired:
                logger.fatal("Timeout attempting to terminate av plugin")
                logger.fatal("Log: %s", self.get_log())
                self._proc.kill()
                self._proc.wait()

            self._proc = None
        if self._failed:
            print("Report on Failure:")
            print(self.get_log())
        logger.debug("Finish stop_av")

    def kill_av(self):
        """
        Stop AV and do not allow graceful termination
        """
        if self._proc:
            self._proc.kill()
            self._proc.wait()
            self._proc = None

    # Will return empty string if log doesn't exist
    def get_log(self):
        log_path = _plugin_log_path()
        if not os.path.exists(log_path):
            return ""
        return _file_content(log_path)

    def log_contains(self, content):
        log_content = self.get_log()
        return content in log_content

    def wait_log_contains(self, content, timeout=10):
        """Wait for up to timeout seconds for the log_contains to report true"""
        for i in range(timeout):
            if self.log_contains(content):
                return True
            time.sleep(1)
        raise AssertionError("Log does not contain {}.\nFull log: {}".format(content, self.get_log()))

    def wait_file(self, relative_path, timeout=10):
        full_path = os.path.join(_sophos_spl_path(), relative_path)
        for i in range(timeout):
            if os.path.exists(full_path):
                return _file_content(full_path)
            time.sleep(1)
        dir_path = os.path.dirname(full_path)
        files_in_dir = os.listdir(dir_path)
        raise AssertionError("File not found after {} seconds. Path={}.\n Files in Directory {} \n Log:\n {}".
                             format(timeout, full_path, files_in_dir, self.get_log()))

    @staticmethod
    def get_latest_xml_from_events(relative_path):
        full_path = os.path.join(_sophos_spl_path(), relative_path)
        xml_files = glob.iglob(f'{full_path}/*.xml')
        latest_xml = max(xml_files, key=os.path.getctime)
        return latest_xml

