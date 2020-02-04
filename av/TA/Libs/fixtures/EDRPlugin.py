import subprocess
import os
import time
import grp
import pwd

def _sophos_spl_path():
    return os.environ['SOPHOS_INSTALL']


def _edr_exec_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/bin/edr')

def _osquery_database_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/var/osquery.db')

def _edr_log_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/log/edr.log')

def _log_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/log/')

def _edr_oquery_paths():
    _edr_log_dir = os.path.join(_sophos_spl_path(), 'plugins/edr/log')
    osquery_logs = [file_name for file_name in os.listdir(_edr_log_dir) if 'osquery' in file_name ]
    if not osquery_logs:
        return []
    return [os.path.join(_edr_log_dir, file_name) for file_name in osquery_logs]


def _file_content(path):
    with open(path, 'r') as f:
        return f.read()


class EDRPlugin:
    def __init__(self):
        self._proc = None
        self._failed = False

    def set_failed(self):
        self._failed = True

    def osquery_database_path(self):
        return _osquery_database_path()

    def log_path(self):
        return _log_path()

    def _ensure_sophos_required_unix_user_and_group_exists(self):
        try:
            grp.getgrnam('sophos-spl-group')
            pwd.getpwnam('sophos-spl-user')
        except KeyError as ex:
            raise AssertionError("Sophos spl group, or user not present: {}".format(ex))

    def prepare_for_test(self):
        self.stop_edr()
        self._ensure_sophos_required_unix_user_and_group_exists()

    def start_edr(self):
        self.prepare_for_test()
        self._proc = subprocess.Popen([_edr_exec_path()])

    def stop_edr(self):
        """
        Stop EDR and allow graceful termination
        """
        if self._proc:
            self._proc.terminate()
            self._proc.wait()
            self._proc = None
        if self._failed:
            print("Report on Failure:")
            print(self.log())
            print("Osquery logs:")
            print(self.osquery_logs())

    def kill_edr(self):
        """
        Stop EDR and do not allow graceful termination
        """
        if self._proc:
            self._proc.kill()
            self._proc.wait()
            self._proc = None

    # Will return empty string if log doesn't exist
    def log(self):
        log_path = _edr_log_path()
        if not os.path.exists(log_path):
            return ""
        return _file_content(log_path)

    def osquery_logs(self):
        full_content = ""
        for file_path in _edr_oquery_paths():
            full_content += "\n\tFile: {}\n".format(file_path)
            full_content += _file_content(file_path)
        return full_content

    def log_contains(self, content):
        log_content = self.log()
        return content in log_content

    def wait_log_contains(self, content, timeout=10):
        """Wait for up to timeout seconds for the log_contains to report true"""
        for i in range(timeout):
            if self.log_contains(content):
                return True
            time.sleep(1)
        raise AssertionError("Log does not contain {}.\nFull log: {}".format(content, self.log()))

    def wait_file(self, relative_path, timeout=10):
        full_path = os.path.join(_sophos_spl_path(), relative_path)
        for i in range(timeout):
            if os.path.exists(full_path):
                return _file_content(full_path)
            time.sleep(1)
        dir_path = os.path.dirname(full_path)
        files_in_dir = os.listdir(dir_path)
        raise AssertionError("File not found after {} seconds. Path={}.\n Files in Directory {} \n Log:\n {}".
                             format(timeout, full_path, files_in_dir, self.log()))


