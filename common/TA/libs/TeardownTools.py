# Copyright 2023-2024 Sophos Limited. All rights reserved.
import datetime
import os
import shutil
import subprocess as sp
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn
import glob
import re


def _list_dirs(root_path):
    try:
        children = [os.path.join(root_path, child) for child in os.listdir(root_path)]
        return [p for p in children if os.path.isdir(p)]
    except OSError as ex:
        logger.info("List failed for {}. Reason: {}".format(root_path, str(ex)))
        return []


def _list_logs(dir_path):
    try:
        file_paths = [os.path.join(dir_path, file_name) for file_name in os.listdir(dir_path)]
        return [file_path for file_path in file_paths if '.log' in file_path]
    except OSError as ex:
        logger.info("List logs failed for {}. Reason: {}".format(dir_path, str(ex)))
        return []


def _plugin_log_files():
    plugins_root = '/opt/sophos-spl/plugins'
    relative_log_path = 'log'
    list_plugins = _list_dirs(plugins_root)
    if not list_plugins:
        return []
    all_logs = []
    for plugin_path in list_plugins:
        logs = _list_logs(os.path.join(plugin_path, relative_log_path))
        if logs:
            all_logs += logs
    return all_logs


def _base_log_files():
    logs_root = '/opt/sophos-spl/logs'
    relative_paths = ['base', 'base/sophosspl']
    logs_dir = [os.path.join(logs_root, relative) for relative in relative_paths]
    all_logs = []
    for log_dir in logs_dir:
        log_paths = _list_logs(log_dir)
        if log_paths:
            all_logs += log_paths
    return all_logs


def _log_files():
    plugin = _plugin_log_files()
    base = _base_log_files()
    if plugin:
        base = base + plugin
    return base


class FailedCase:
    def __init__(self, line, path):
        self.line = line
        self.path = path

    def __str__(self):
        return "Error in file: {}. Line: {}".format(self.path, self.line)


class TeardownTools(object):
    ROBOT_LIBRARY_SCOPE = 'TEST SUITE'
    ROBOT_LISTENER_API_VERSION = 2

    FORCE_LOGGING_KEY="FORCE_LOGGING"

    def __init__(self):
        self.log_files = _log_files()
        self.log_mark_dict = {}
        for log_key in self.log_files:
            self.log_mark_dict[log_key] = None

    def force_teardown_logging_if_env_set(self):
        if not os.environ.get(self.FORCE_LOGGING_KEY, None):
            logger.info("{} not set".format(self.FORCE_LOGGING_KEY))
            return

        BuiltIn().run_keyword_if_test_passed("Dump All Logs")
        BuiltIn().run_keyword_if_test_passed("Log Status Of Sophos Spl")
        BuiltIn().run_keyword_if_test_passed("Display All SSPL Files Installed")
        BuiltIn().run_keyword_if_test_passed("Dump All Sophos Processes")

    def dump_teardown_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "r") as f:
                    logger.info(''.join(f.readlines()))
            except UnicodeDecodeError as ex:
                logger.error("Failed to decode {} as UTF-8: {}".format(filename, ex))
                with open(filename, "rb") as f:
                    data = f.read()
                    data = data.decode("LATIN-1")
                    logger.info("Contents as Latin-1:" + data)
            except Exception as ex:
                logger.error("Failed to read file: {}".format(ex))
        else:
            logger.info("File {} does not exist".format(filename))

    def analyse_Journalctl(self, print_always):
        if isinstance(print_always, bool):
            to_print = print_always
        else:
            if str(print_always).lower() in ['true', '1']:
                to_print = True
            else:
                to_print = False
        failures = []
        try:
            out = sp.check_output(['journalctl', '--no-pager', '--since', '5 minutes ago'],
                                  stdin=open('/dev/null', 'wb'))
            output = out.decode('utf-8')
            if to_print:
                logger.info(output)
            failures = []
            for line in output.split('\n'):
                if 'Critical unhandled' in line:
                    failures.append(line)
        except Exception as ex:
            logger.warn(str(ex))
        if failures:
            raise AssertionError("Unhandled exceptions found in {}".format(failures))

    def mark_log_file(self, file_path):
        with open(file_path, "r") as file:
            last_line = file.readlines()[-1]
            self.log_mark_dict[file_path] = last_line
            logger.info("Marked log: \"{}\" \nwith\t \"{}\"".format(file_path, last_line))

    def require_no_unhandled_exception(self):
        trigger_entry = []
        for file_path, mark in self.log_mark_dict.items():
            logger.debug("Checking log file for unhandled exception: {}".format(file_path))
            try:
                with open(file_path, 'r') as file_handler:
                    for line in reversed(file_handler.readlines()):
                        if mark and line == mark:
                            logger.info("stopping at mark: {}".format(mark))
                            break
                        if 'Caught exception at top-level' in line:
                            trigger_entry.append(FailedCase(line, file_path))
                        if 'Critical unhandled' in line:
                            trigger_entry.append(FailedCase(line, file_path))
                self.mark_log_file(file_path)
            except Exception as ex:
                logger.info("Parsing file {} failed. Reason: {}".format(file_path, ex))
        if trigger_entry:
            raise AssertionError('\n'.join([str(e) for e in trigger_entry]))

    def _run_combine_ignore_error(self, files):
        try:
            args = ["python3", "-m", "coverage", "combine"] + files
            logger.info("Combine the following files: {}".format(files))
            _process = sp.Popen(args, stdout=sp.PIPE)
            _process.wait()
            stdout, stderr = _process.communicate()

            if stderr:
                logger.warn(stderr)
            if stdout:
                logger.info("Coverage combine output: {}".format(stdout))
            if _process.returncode == 0:
                logger.info("Coverages combined into {}".format(os.path.abspath('.coverage')))
        except Exception as ex:
            logger.info("Error on combining coverage {}".format(ex))

    def combine_coverage_if_present(self):
        def get_paths(path):
            entries = glob.glob(path + '*')
            return [os.path.abspath(entry) for entry in entries]
        root_coverage = get_paths('/opt/sophos-spl/tmp/.coverage')
        tests_coverage = get_paths('.coverage')
        if root_coverage or tests_coverage:
            files = root_coverage + tests_coverage
            self._run_combine_ignore_error(files)

    def copy_to_filer6_or_s3_pickup(self, filepath, testname,filer6):
        if filer6:
            core_output_dir = "/mnt/filer6/linux/SSPL/CoreDumps"
        else:
            core_output_dir = "/opt/test/coredumps"
        testname = (testname + "_" + str(datetime.datetime.now())).replace(" ", "_")
        core_dump_dir = os.path.join(core_output_dir, testname)

        logger.info("copying file: {} to {}".format(filepath,core_output_dir))
        if not os.path.exists(core_dump_dir):
            os.makedirs(core_dump_dir)
        shutil.copy(filepath, core_dump_dir)

    def mount_filer6_DISABLED(self):
        filer6 = "/mnt/filer6/linux"
        sspl_filer_path = "/mnt/filer6/linux/SSPL"

        if not os.path.ismount(sspl_filer_path):
            popen = sp.Popen(["mount", filer6], stdout=sp.PIPE, stderr=sp.PIPE)
            output, stderror = popen.communicate()

            if output:
                logger.info(output)

            if stderror:
                logger.info(stderror)

            returncode = popen.returncode
            if returncode != 0:
                return returncode
    
