import os
import json
import base64
import shutil
import subprocess
import time
from Watchdog import get_install
import PathManager

try:
    raise 'skip'
    from robot.api import logger
except:
    import logging
    logging.basicConfig(level=logging.DEBUG)
    logger = logging.getLogger('test')


MOCKSERVICEDIR='/tmp/test/mock_darkbytes/'

def _dbos_log_dir():
    return os.path.join(get_install(), "plugins/mtr/dbos/data/logs")

def _cert_path():
    return os.path.join(MOCKSERVICEDIR, 'server.crt')

def _mtr_plugin_path():
    return os.path.join(get_install(), "plugins/mtr/log/mtr.log")

def _dbos_log_path():
    return os.path.join(_dbos_log_dir(), "dbos.log")

def _watcher_log_path():
    return os.path.join(_dbos_log_dir(), "osquery.watcher.log")

def _osquery_log_paths():
    return [os.path.join(get_install(), 'plugins/mtr/dbos/data/logs/', file_name) for file_name in
     ['osqueryd.INFO', 'osqueryd.WARNING', 'osqueryd.results.log']]

def _osquery_verbose_log():
    return os.path.join(_dbos_log_dir(), "osqueryd.output.log")

def _mtr_logs():
    logs =_osquery_log_paths() + [_dbos_log_path(), _watcher_log_path(), _mtr_plugin_path()]
    return logs

def _action_dir():
    return os.path.join(MOCKSERVICEDIR, 'results/actions')

def _scheduled_queries_dir():
    return os.path.join(MOCKSERVICEDIR, 'results/running_processes')

def _live_query_request_path():
    return os.path.join(MOCKSERVICEDIR, 'distributed/live_query.txt')

def _live_query_results_path():
    return os.path.join(MOCKSERVICEDIR, 'distributed/results')


def _mtrmockservice_log_paths():
    return [os.path.join(MOCKSERVICEDIR, 'mtrservice.log'), os.path.join(MOCKSERVICEDIR, 'osquery_server.log')]


def _write_to_file(file_path, content):
    with open(file_path, 'w') as file_handler:
        file_handler.write(content)

def _read_file(file_path):
    with open(file_path, 'r') as file_handler:
        return file_handler.read()

def _encode_cmd(content):
    cmd = base64.b64encode(content.encode('utf-8'))
    cmd = cmd.decode('utf-8')
    return cmd


def json_line_parser(line):
    return json.loads(line)

def osquery_verbose_log_line_parser(line):
    if len(line) == 0:
        return {}
    entry = {'err': '', 'caller': '', 'msg': ''}
    if line[0] != 'I':
        entry['err'] = line
    elif 'Error' in line:
        entry['err'] = line
    return entry



class MTRService(object):
    """Mainly created to enable RobotTest.
    """
    def __init__(self):
        self.pending_request = os.path.join(MOCKSERVICEDIR, 'newrequest.txt')
        self.pending_raw_request = os.path.join(MOCKSERVICEDIR, 'newrawrequest.txt')
        self.support_path = PathManager.get_support_file_path()
        self.mtr_path = os.path.join(self.support_path, "runDarkBytesMock.py")
        self.mtr_service = None
        self.mtr_log = None

    def __del__(self):
        self.stop_mtrservice()

    def _setup_testpath(self):
        if not os.path.exists(MOCKSERVICEDIR):
            os.makedirs(MOCKSERVICEDIR)
        for file_path in _mtrmockservice_log_paths():
            if os.path.exists(file_path):
                os.remove(file_path)

    def _count_log_errors(self, file_path, line_parser, ignore_list=[]):
        count = 0
        logger.info("Check file: {}".format(file_path))
        with open(file_path, 'r') as dbos_handler:
            for line in dbos_handler:
                dict_entry = line_parser(line)
                err = dict_entry.get('err', '')
                if err:
                    caller = dict_entry.get('caller', '')
                    extra_msg = dict_entry.get('msg', '')
                    ignored = False
                    for ignore_entry in ignore_list:
                        if ignore_entry in extra_msg or ignore_entry in err:
                            ignored = True
                            break
                    content = "Error: {}, Msg: {}, Reported by: {}".format(err, extra_msg, caller)

                    if ignored:
                        logger.debug("Ignored error {}".format(content))
                        continue
                    print(content)
                    logger.info(content)
                    count = count + 1
        return count

    def _count_live_terminal_requests(self):
        string_to_match = """'id': 'liveterminal'"""
        count = 0
        with open(_mtrmockservice_log_paths()[1], 'r') as osquery_server_log_handler:
            for line in osquery_server_log_handler:
                if string_to_match in line:
                    count = count + 1
        return count

    def _log_contents(self):
        std_out = _read_file(_mtrmockservice_log_paths()[0])
        file_out = _read_file(_mtrmockservice_log_paths()[1])
        return std_out + file_out

    def _add_etc_hosts_entry(self):
        with open('/etc/hosts', 'r') as etc_hosts:
            with open('/tmp/hosts', 'w') as copy_etc_hosts:
                for line in etc_hosts:
                    if not 'endpointintel.darkbytes.io' in line:
                        copy_etc_hosts.write(line)
                copy_etc_hosts.write('127.0.0.1   test.endpointintel.darkbytes.io\n')
        return subprocess.check_output(['sudo', 'mv', '/tmp/hosts', '/etc/hosts'])

    def _remove_etc_hosts_entry(self):
        with open('/etc/hosts', 'r') as etc_hosts:
            with open('/tmp/hosts', 'w') as copy_etc_hosts:
                for line in etc_hosts:
                    if 'endpointintel.darkbytes.io' in line:
                        continue
                    copy_etc_hosts.write(line)
        return subprocess.check_output(['sudo', 'mv', '/tmp/hosts', '/etc/hosts'])

    def _check_can_curl(self):
        cert_path = _cert_path()
        cmd = ["curl", '--cacert', cert_path, '--capath', MOCKSERVICEDIR, 'https://test.endpointintel.darkbytes.io']
        logger.info("Run command: {}".format(cmd))
        try:
            output = subprocess.check_output(cmd)
            logger.debug("Output: {}".format(output))
        except subprocess.CalledProcessError as ex:
            logger.warn("Failure checking service: {}. Output: {}".format(ex, ex.output))
            raise

    def _set_of_files_in_directory(self, directory_path):
        if os.path.exists(directory_path):
            current_result_files = os.listdir(directory_path)
        else:
            current_result_files = []
        return set(current_result_files)

    def _curr_action_files(self):
        action_dir = _action_dir()
        return self._set_of_files_in_directory(action_dir)

    def _curr_scheduled_queries_answers(self):
        return self._set_of_files_in_directory(_scheduled_queries_dir())

    def _curr_live_queries_answers(self):
        return self._set_of_files_in_directory(_live_query_results_path())


    def start_mtrservice(self):
        """Most likely the first action in any test. It will ensure the machine is configured and the
        Mock Darkbytes WebApp is launched.
        """
        logger.info("Starting MTRService: {}".format(self.mtr_path))
        self._setup_testpath()
        if not os.path.exists(self.mtr_path):
            raise os.IOError("File does not exist {}".format(self.mtr_path))
        for dir_name in ['results', 'distributed', 'statuses']:
            dir_path = os.path.join(MOCKSERVICEDIR, dir_name)
            if os.path.exists(dir_path):
                logger.info("Cleaning directory: {}".format(dir_path))
                shutil.rmtree(dir_path)
        command = os.path.abspath(self.mtr_path)
        logger.info("Starting MTRService: {}".format(command))
        self.mtr_log = open(_mtrmockservice_log_paths()[0], 'w')
        self.mtr_service = subprocess.Popen(command, stdout=self.mtr_log, stderr=self.mtr_log)
        time.sleep(1)
        if self.mtr_service.poll():
            # it has already finished. which is bad
            self.stop_mtrservice()
            content = _read_file(_mtrmockservice_log_paths()[0])
            raise AssertionError("Failed to start: {}".format(content))
        else:
            self._add_etc_hosts_entry()
        self._check_can_curl()

    def stop_mtrservice(self):
        """Stop the Mock Darkbytes WebApp and restore settings that were changed in the start_mtrservice"""
        logger.info("Stopping MTRService ")
        self._remove_etc_hosts_entry()
        if self.mtr_service:
            if self.mtr_service.poll() is None:
                self.mtr_service.kill()
            self.mtr_service = None
        if self.mtr_log:
            self.mtr_log.close()

    def fix_dbos_certs_and_restart(self):
        """The MockDarkbytes server is running with a different certificate than the official darkbytes.
        Hence, the mtr plugin will fail.
        This method, stops the mtr plugin, replace the certificate(forcing the replacement), blank logs and restart mtr.
        This allows to verify no other problem occur."""
        dbos_cert_path = os.path.join(get_install(), "plugins/mtr/dbos/data/certificate.crt")
        _cert_path()
        wdctl=os.path.join(get_install(), 'bin/wdctl')
        subprocess.check_output(['sudo', wdctl, 'stop', 'mtr'])
        self.clear_dbos_logs()
        subprocess.check_output(['sudo', 'cp', _cert_path(), dbos_cert_path])
        subprocess.check_output(['sudo', wdctl, 'start', 'mtr'])
        subprocess.check_output(['sudo', wdctl, 'start', 'mtr'])

    def check_mtrconsole_can_send_action(self, command, timeout=200):
        logger.info("Scheduler action: {}".format(command))
        start_actions = self._curr_action_files()
        _write_to_file(self.pending_request, command)
        currtime = time.time()
        futuretime = currtime + timeout
        passed = False
        while time.time() < futuretime:
            time.sleep(1)
            curr_actions = self._curr_action_files()
            new_actions = curr_actions - start_actions
            if new_actions:
                passed = True
                for action_file in new_actions:
                    content = _read_file(os.path.join(_action_dir(), action_file))
                    logger.info("Action responded. Content: {}".format(content))
                break
        if not passed:
            raise AssertionError("Failed to get reply for the command {}".format(command))

    def check_mtrconsole_can_delete_file(self, timeout=200):
        file_path = '/tmp/just_an_example.txt'
        _write_to_file(file_path, 'anything')
        if not os.path.exists(file_path):
            raise AssertionError("Failed to create the file {}".format(file_path))
        logger.info("Scheduler delete file: {}".format(file_path))
        cmd = {'id': 'delete', 'actionName': 'delete_threat_file', 'command': _encode_cmd(file_path), 'documentId': 'file_path_id'}
        encoded_response = json.dumps(cmd)
        _write_to_file(self.pending_raw_request, encoded_response)
        currtime = time.time()
        futuretime = currtime + timeout
        passed = False
        while time.time() < futuretime:
            time.sleep(1)
            if not os.path.exists(file_path):
                passed = True
                break
        if not passed:
            raise AssertionError("Failed to instruct SophosMTR to delete a path")

    def check_mtrconsole_can_trigger_live_terminal(self, timeout=200):
        """The current version of runDarkBytesMock does not support liveterminal. Hence, this will just detect the instruction
        was received, not the liveterminal itself."""
        logger.info("Scheduler simulate live terminal")
        cmd = {'id': 'liveterminal', 'actionName': 'live_terminal', 'documentId': 'file_path_id', 'params': {'sessionId':'mytest'}}
        encoded_response = json.dumps(cmd)
        current_count_of_dbos_errors = self.count_dbos_log_errors_default_ignore()
        beforetest_live_terminal_requests = self._count_live_terminal_requests()
        _write_to_file(self.pending_raw_request, encoded_response)
        currtime = time.time()
        futuretime = currtime + timeout
        passed = False
        while time.time() < futuretime:
            time.sleep(3)
            live_terminal_requests = self._count_live_terminal_requests()
            if live_terminal_requests > beforetest_live_terminal_requests:
                passed = True
                break
        new_errors = self.count_dbos_log_errors_default_ignore()
        if not passed:
            raise AssertionError("""Failed to instruct SophosMTR to trigger new live terminal. 
CountErrors Initially {}, current: {}. Num of terminal requests: {}""".format(current_count_of_dbos_errors, new_errors, live_terminal_requests))

    def clear_dbos_logs(self):
        paths_to_remove = _mtr_logs()
        subprocess.call(['sudo', 'rm', '-rf'] + paths_to_remove)

    def count_dbos_log_errors_default_ignore(self, extra_ingore=[]):
        count = self._count_log_errors(_dbos_log_path(), json_line_parser,  ignore_list=['No orphaned Osquery Process'] + extra_ingore)
        count = count + self._count_log_errors(_watcher_log_path(), json_line_parser)
        return count

    def count_mtr_log_errors(self):
        count = self._count_log_errors(_dbos_log_path(), json_line_parser,  ignore_list=['No orphaned Osquery Process'])
        count = count + self._count_log_errors(_watcher_log_path(), json_line_parser)
        for osquery_file_path in _osquery_log_paths():
            if not os.path.exists(osquery_file_path):
                logger.info("Skipping file {} as it is not present.".format(osquery_file_path))
                continue
            count = count + self._count_log_errors(osquery_file_path, json_line_parser, ignore_list=[])
        return count

    def count_osquery_verbose_errors(self, ignore_list=[]):
        ignore_list.append('osquery::Killswitch::IsEnabledError')
        ignore_list.append('Error registering subscriber: process_file_events')
        ignore_list.append('Error registering subscriber: selinux_events')
        ignore_list.append('Error registering subscriber: socket_events')
        return self._count_log_errors(_osquery_verbose_log(), osquery_verbose_log_line_parser, ignore_list)

    def check_osquery_is_sending_scheduled_query_results(self):
        currtime = time.time()
        futuretime = currtime + 200
        before_scheduled_results = self._curr_scheduled_queries_answers()
        passed = False
        while time.time() < futuretime:
            time.sleep(3)
            scheduled_results = self._curr_scheduled_queries_answers()
            new_ones = scheduled_results - before_scheduled_results
            if new_ones:
                passed = True
                for file_name in new_ones:
                    full_path = os.path.join(_scheduled_queries_dir(), file_name)
                    file_content = _read_file(full_path)
                    logger.info("Scheduled Query result: File: {}, Content: {}".format(file_name, file_content))
                break
        if not passed:
            raise AssertionError("Failed to detect schedule query results are being published.")

    def check_osquery_respond_to_livequery(self):
        livequery_file_path = _live_query_request_path
        before_live_query_results = self._curr_live_queries_answers()
        logger.debug("There are {} current queries".format(len(before_live_query_results)))
        logger.debug("Write query to file")
        _write_to_file(livequery_file_path(), 'SELECT name,value from osquery_flags;')
        currtime = time.time()
        futuretime = currtime + 200
        passed = False
        while time.time() < futuretime:
            time.sleep(3)
            after_livequery_results = self._curr_live_queries_answers()
            new_ones = after_livequery_results - before_live_query_results
            if new_ones:
                logger.debug("New answers found. {}".format(len(new_ones)))
                for file_name in new_ones:
                    logger.debug("Checking file: {}".format(file_name))
                    full_path = os.path.join(_live_query_results_path(), file_name)
                    file_content = _read_file(full_path)
                    logger.info("Livequery results: File: {}".format(file_name))
                    if 'watchdog_memory_limit' in file_content:
                        passed = True
                if passed:
                    break

        if not passed:
            raise AssertionError("Failed to detect live query results were responded.")


    def dump_mtr_service_logs(self):
        logger.info(self._log_contents())
