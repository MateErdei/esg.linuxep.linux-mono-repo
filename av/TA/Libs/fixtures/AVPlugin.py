import subprocess
import os
import time
import grp
import pwd
import glob
import xml.etree.ElementTree

try:
    from . import Paths
    from . import TestBase
except ImportError:
    import Paths
    import TestBase

import logging
logger = logging.getLogger("AVPlugin")
logger.setLevel(logging.DEBUG)


def _sophos_spl_path():
    return Paths.sophos_spl_path()


PLUGIN_NAME = "av"


def _av_plugin_dir():
    return Paths.av_plugin_dir()


def _av_exec_path():
    return os.path.join(_av_plugin_dir(), 'sbin', PLUGIN_NAME)


def _log_path():
    return os.path.join(_av_plugin_dir(), 'log')


def _plugin_log_path():
    return os.path.join(Paths.log_path(), PLUGIN_NAME+'.log')


def _file_content(path):
    with open(path, 'r') as f:
        return f.read()

def _is_event_xml(path):
    tree = xml.etree.ElementTree.parse(path)
    root = tree.getroot()
    if root.tag == "event" or root.tag == "{http://www.sophos.com/EE/EESavEvent}event":
        return True
    logger.info("%s has root element: %s", path, root.tag)
    return False


class AVPlugin(TestBase.TestBase):
    def __init__(self):
        super().__init__()
        self._proc = None
        self._failed = False

    def set_failed(self):
        self._failed = True

    def plugin_path(self):
        return Paths.av_plugin_dir()

    def log_path(self):
        return Paths.log_path()

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
        output = subprocess.check_output(['ldd', _av_exec_path()]) ## check we can load all the libraries
        logger.debug("ldd output: %s", output)
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

    @staticmethod
    def get_latest_xml_from_events(relative_path):
        full_path = os.path.join(_sophos_spl_path(), relative_path)
        xml_files = glob.iglob(f'{full_path}/*.xml')
        latest_xml = max(xml_files, key=os.path.getctime)
        return latest_xml

    @staticmethod
    def get_latest_event_xml_from_events(relative_path, after=None):
        if after is None or after == "":
            after = 0
        else:
            after = int(after)

        full_path = os.path.join(_sophos_spl_path(), relative_path)
        xml_files = glob.iglob(f'{full_path}/SAV_event-*.xml')
        current_xml_files = [ x for x in xml_files if os.path.getctime(x) >= after ]
        if len(current_xml_files) == 0:
            logger.fatal("All %d XML files older than %d", len(xml_files), after)
            raise Exception("No XML files after %d", after)

        xml_files = current_xml_files
        xml_files.sort(key=os.path.getctime, reverse=True)
        logger.debug("Found %d XML files", len(xml_files))
        for xml_file in xml_files:
            if _is_event_xml(xml_file):
                logger.debug("Found Event XML %s", xml_file)
                return xml_file
            else:
                logger.info("Ignoring %s as not event XML", xml_file)
        logger.fatal("No event XML found")
        raise Exception("No Event XML found")
