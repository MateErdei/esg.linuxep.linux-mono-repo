import subprocess
import os
import time

def _sophos_spl_path():
    return os.environ['SOPHOS_INSTALL']

def _edr_exec_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/bin/edr')

def _edr_log_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/log/edr.log')


def _file_content(path):
    with open(path, 'r') as f:
        return f.read()


class EDRPlugin:
    def __init__(self):
        self._proc = None

    def start_edr(self):
        self.stop_edr()
        self._proc = subprocess.Popen([_edr_exec_path()])

    def stop_edr(self):
        if self._proc:
            self._proc.kill()
            self._proc.wait()
            self._proc = None

    def log(self):
        return _file_content(_edr_log_path())

    def log_contains(self, content):
        log_content = self.log()
        return content in log_content

    def wait_log_contains(self, content, timeout=10):
        """Wait for up to timeout seconds for the log_contains to report true"""
        for i in range(timeout):
            if self.log_contains(content):
                return
            time.sleep(1)
        raise AssertionError("Log does not contain {}.\nFull log: {}".format(content, self.log()))

    def wait_file(self, relative_path, timeout=10):
        full_path = os.path.join(_sophos_spl_path(), relative_path)
        for i in range(timeout):
            if os.path.exists(full_path):
                return _file_content(full_path)
            time.sleep(1)
        raise AssertionError("File not found after {} seconds. Path={}".format(timeout, full_path))


