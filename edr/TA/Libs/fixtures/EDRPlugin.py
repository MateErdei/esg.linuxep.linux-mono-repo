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

def _log_path():
    return os.path.join(_sophos_spl_path(), 'plugins/edr/log/')

def _edr_log_path():
    return os.path.join(_log_path(), 'edr.log')

def _live_query_log_path():
    return os.path.join(_log_path(), 'livequery.log')

def _edr_oquery_paths():
    _edr_log_dir = _log_path()
    osquery_logs = [file_name for file_name in os.listdir(_edr_log_dir) if 'osquery' in file_name ]
    if not osquery_logs:
        return []
    return [os.path.join(_edr_log_dir, file_name) for file_name in osquery_logs]


def _file_content(path):
    with open(path, 'r') as f:
        return f.read()


def _try_file_content(file_path):
    try:
        with open(file_path, 'r') as file_handler:
            return file_handler.read()
    except IOError:
        # reading proc entry can fail related to the time to read
        # ignore those errors
        return ""


class ProcEntry:
    def __init__(self, pid):
        self.pid = int(pid)
        self._name = _try_file_content(os.path.join('/proc', str(pid), 'comm')).strip()

    def name(self):
        return self._name

    def __repr__(self):
        return "pid: {}, name:{}".format(self.pid, self._name)


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
        self.wait_for_osquery_to_run()

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

    def process_iter(self):
        for pid in os.listdir('/proc'):
            if '0' <= pid[0] <= '9':
                yield ProcEntry(pid)

    def wait_for_osquery_to_run(self):
        times_run = 0
        while times_run < 60:
            times_run += 1
            for p in self.process_iter():
                if p.name() == "osqueryd":
                    return p.pid
            time.sleep(1)
        raise AssertionError("osqueryd not found in process list: {}".format([p for p in process_iter()]))

    def wait_for_osquery_to_stop(self, pid):
        times_run = 0
        while times_run < 60:
            times_run += 1
            pids = []
            for p in self.process_iter():
                pids.append(p.pid)

            if pid in pids:
                time.sleep(1)
                continue
            return
        with open('/proc/{}/status'.format(pid)) as handler:
            content = handler.read()
        raise AssertionError("osqueryd failed to stop. Information {}".format(content))

    # Will return empty string if log doesn't exist
    def log(self):
        log_path = _edr_log_path()
        live_query_path = _live_query_log_path()
        content=[]
        for path in [log_path, live_query_path]:
            if os.path.exists(path):
                content.append(_file_content(path))
        return '\n'.join(content)

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


