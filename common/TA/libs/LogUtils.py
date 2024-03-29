# Copyright 2023-2024 Sophos Limited. All rights reserved.

import glob
import os
import re
import shutil
import subprocess
import time
import typing

import dateutil.parser
import robot.libraries.BuiltIn
from robot.api import logger

try:
    from . import LogHandler
except ImportError:
    import LogHandler


_ensure_str = LogHandler.ensure_unicode

def _get_log_contents(path_to_log):
    try:
        with open(path_to_log, "r") as log:
            contents = log.read()
            return contents
    except UnicodeDecodeError as ex:
        logger.error("Failed to read {} in UTF-8: {}".format(path_to_log, ex))
        with open(path_to_log, "rb") as log:
            contents = log.read()
            contents = contents.decode("LATIN-1")
            logger.info("Contents of {} as Latin-1: {}".format(path_to_log, contents))
            return contents


def get_log_length(path_to_log):
    if not os.path.isfile(path_to_log):
        return 0
    with open(path_to_log, "r") as log:
        contents = log.read()
    return len(contents)


def log_contains_in_order(log_location, log_name, args, log_finds=True):
    contents = _get_log_contents(log_location)
    index = 0
    for string in args:
        index = contents.find(string, index)
        if index != -1:
            if log_finds:
                logger.info(f"{log_name} log contains {string}")
            index = index + len(string)
        else:
            raise AssertionError(f"Remainder of {log_name} log doesn't contain {string}")


def get_variable(var_name, default_value=None):
    try:
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % var_name, default_value)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


def _mark_expected_lines_in_log(log_location, error_string, mark_string, *error_messages):
    changed = False
    for logfile in glob.glob(log_location + "*"):
        logger.info("Attempting to mark: " + logfile)
        contents = _get_log_contents(logfile)
        if contents is None:
            logger.debug("File {} is empty no errors to exclude!".format(logfile))
            continue

        original_contents = contents

        old_lines = contents.splitlines()
        new_lines = []

        for line in old_lines:
            for error_message in error_messages:
                if error_message in line:
                    line = line.replace(error_string, mark_string)
                    break  # Don't need to look any further
            new_lines.append(line + "\n")
        contents = u"".join(new_lines)

        if contents != original_contents:
            logger.info("Marking: " + logfile)
            with open(logfile, "wb") as log:
                log.write(contents.encode("UTF-8"))
            changed = True

    return changed


def _mark_expected_errors_in_log(log_location, *error_messages):
    error_string = "ERROR"
    mark_string = "expected-error"
    return _mark_expected_lines_in_log(log_location, error_string, mark_string, *error_messages)


class LogUtils(object):
    def __init__(self):
        self.tmp_path = os.path.join(".", "tmp")
        self.__set_log_paths()
        self.cloud_server_log = os.path.join(self.tmp_path, "cloudServer.log")
        self.marked_mcsrouter_logs = 0
        self.marked_mcs_envelope_logs = 0
        self.marked_watchdog_logs = 0
        self.marked_managementagent_logs = 0
        self.marked_av_log = 0
        self.marked_sophos_threat_detector_log = 0
        self.marked_susi_debug_log = 0
        self.marked_ss_log = 0
        self.marked_scan_now_log = 0
        self.marked_on_access_log = 0
        self.marked_edr_log = 0
        self.marked_edr_osquery_log = 0
        self.marked_livequery_log = 0
        self.marked_managementagent_log = 0
        self.marked_safestore_log = 0
        self.marked_sul_logs = 0
        self.marked_update_scheduler_logs = 0
        self.__m_marked_log_position = {}

        self.__m_pending_mark_expected_errors = {}
        self.__m_log_handlers = {}

    def __set_log_paths(self, install_path=None):
        if install_path is None:
            # Get Robot variable
            install_path = get_variable("SOPHOS_INSTALL", os.path.join("/", "opt", "sophos-spl"))

        self.install_path = install_path
        self.router_path = os.path.join(self.install_path, "base", "mcs")
        self.base_logs_dir = os.path.join(self.install_path, "logs", "base")
        self.register_log = os.path.join(self.base_logs_dir, "register_central.log")
        self.suldownloader_log = os.path.join(self.base_logs_dir, "suldownloader.log")
        self.watchdog_log = os.path.join(self.base_logs_dir, "watchdog.log")
        self.wdctl_log = os.path.join(self.base_logs_dir, "wdctl.log")
        self.sdu_log = os.path.join(self.base_logs_dir, "sophosspl", "remote_diagnose.log")
        self.diagnose_log = os.path.join(self.base_logs_dir, "diagnose.log")
        self.managementagent_log = os.path.join(self.base_logs_dir, "sophosspl", "sophos_managementagent.log")
        self.mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.mcs_router_log = os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")
        self.telemetry_log = os.path.join(self.base_logs_dir, "sophosspl", "telemetry.log")
        self.tscheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "tscheduler.log")
        self.update_scheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "updatescheduler.log")
        self.edr_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr.log")
        self.edr_osquery_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr_osquery.log")
        self.scheduled_query_log = os.path.join(self.install_path, "plugins", "edr", "log", "scheduledquery.log")
        self.livequery_log = os.path.join(self.install_path, "plugins", "edr", "log", "livequery.log")
        self.ej_log = os.path.join(self.install_path, "plugins", "eventjournaler", "log", "eventjournaler.log")
        self.deviceisolation_log = os.path.join(self.install_path, "plugins", "deviceisolation", "log", "deviceisolation.log")
        self.liveresponse_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "liveresponse.log")
        self.sessions_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "sessions.log")
        self.thin_install_log = os.path.join(self.tmp_path, "thin_installer", "ThinInstaller.log")
        self.rtd_log = os.path.join(self.install_path, "plugins", "runtimedetections", "log", "runtimedetections.log")
        self.responseactions_log = os.path.join(self.install_path, "plugins", "responseactions", "log", "responseactions.log")

        # SSPL-AV chroot log files
        self.__m_chroot_logs_dir = os.path.join(self.install_path, "plugins", "av", "chroot", "log")
        self.sophos_threat_detector_log = os.path.join(self.__m_chroot_logs_dir, "sophos_threat_detector.log")
        self.susi_debug_log = os.path.join(self.__m_chroot_logs_dir, "susi_debug.log")

        # SSPL-AV main log files
        self.av_plugin_logs_dir = os.path.join(self.install_path, "plugins", "av", "log")
        self.av_log = os.path.join(self.av_plugin_logs_dir, "av.log")
        self.oa_log = os.path.join(self.av_plugin_logs_dir, "soapd.log")
        self.safestore_log = os.path.join(self.av_plugin_logs_dir, "safestore.log")
        self.scheduled_scan_log = os.path.join(self.av_plugin_logs_dir, "Sophos Cloud Scheduled Scan.log")
        self.scan_now_log = os.path.join(self.av_plugin_logs_dir, "Scan Now.log")

    # Common Log Utils
    def get_log_contents_from_path_to_log(self, path_to_log):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {path_to_log} does not exist")

        with open(path_to_log, "r") as log:
            contents = log.readlines()

        return contents

    def get_log_line(self, string_to_contain, log_mark_or_path_to_log):
        if isinstance(log_mark_or_path_to_log, LogHandler.LogMark):
            path_to_log = log_mark_or_path_to_log.get_path()
            contents_bytes = self.get_log_after_mark(path_to_log, log_mark_or_path_to_log)
            contents_string = contents_bytes.decode("utf-8")
            contents = contents_string.split('\n')
        else:
            contents = self.get_log_contents_from_path_to_log(log_mark_or_path_to_log)

        for line in contents:
            if string_to_contain in line:
                return line

        raise AssertionError(f"Could not find: {string_to_contain} in log given")

    def get_timestamp_of_log_line(self, string_to_contain, log_mark_or_path_to_log):
        line = self.get_log_line(string_to_contain, log_mark_or_path_to_log)
        timestamp = line.split("[")[1].split("]")[0]

        return dateutil.parser.isoparse(timestamp)

    def get_time_difference_between_two_log_lines(self, string_to_contain1, string_to_contain2, log_mark_or_path_to_log):
        timestamp1 = self.get_timestamp_of_log_line(string_to_contain1, log_mark_or_path_to_log)
        timestamp2 = self.get_timestamp_of_log_line(string_to_contain2, log_mark_or_path_to_log)

        difference = timestamp2 - timestamp1

        return difference.total_seconds()

    def get_timestamp_of_next_occurrence_log_b_after_log_a(self,log_location, log_a, log_b):
        contents = []
        with open(log_location, "r") as log:
            contents = log.readlines()

        i = 0
        max_index = len(contents)
        while i < max_index:
            if log_a in contents[i]:
                break
            i += 1

        if i == (max_index - 1):
            raise AssertionError(f"{log_location} log doesn't contain {log_a}")

        while i < max_index:
            if log_b in contents[i]:
                break
            i += 1

        if i == (max_index - 1):
            raise AssertionError(f"Remainder of {log_location} log doesn't contain {log_b}")

        return self.get_timestamp_of_log_line(contents[i], log_location)

    def get_time_difference_between_two_log_lines_where_log_lines_are_in_order(self, string_to_contain1,
                                                                               string_to_contain2,
                                                                               path_to_log):

        timestamp1 = self.get_timestamp_of_log_line(string_to_contain1, path_to_log)
        timestamp2 = self.get_timestamp_of_next_occurrence_log_b_after_log_a(path_to_log,
                                                                            string_to_contain1,
                                                                            string_to_contain2)
        difference = timestamp2 - timestamp1

        return difference.total_seconds()

    def get_number_of_occurrences_of_substring_in_log(self, log_location, substring):
        contents = _get_log_contents(log_location)
        return self.get_number_of_occurrences_of_substring_in_string(contents, substring)

    def get_number_of_occurrences_of_substring_in_string(self, string, substring, use_regex=False):
        if use_regex:
            return self.get_number_of_occurrences_of_regex_in_string(string, substring)
        count = 0
        index = 0
        while True:
            index = string.find(substring, index)
            if index == -1:
                break
            index += len(substring)
            count += 1
        return count

    def check_string_matching_regex_in_file(self, file_path, reg_expression_str):
        if not os.path.exists(file_path):
            raise AssertionError(f"File not found '{file_path}'")
        if self.get_number_of_occurrences_of_regex_in_string(_get_log_contents(file_path), reg_expression_str) < 1:
            self.dump_log(file_path)
            raise AssertionError(f"The file: '{file_path}', did not have any lines match the regex: '{reg_expression_str}'")

    # require that special characters are escaped with '\' [ /, +, *, ., (, ) etc ]
    def get_number_of_occurrences_of_regex_in_string(self, string, reg_expression_str):
        import re
        reg_expression = re.compile(reg_expression_str)
        log_occurrences = reg_expression.findall(string)
        return len(log_occurrences)

    def all_should_be_equal(self, *args):
        assert len(args) > 1, "Error: should have more than 1 argument"
        master = args[0]
        all_equal = True
        for arg in args[1:]:
            if arg != master:
                raise AssertionError(f"Not all items are equal in: {args}")

    def dump_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "r") as f:
                    logger.info(f"file: {str(filename)}")
                    logger.info(''.join(f.readlines()))
                return True
            except UnicodeDecodeError as ex:
                logger.error("Failed to decode {} as UTF-8: {}".format(filename, ex))
                with open(filename, "rb") as f:
                    data = f.read()
                    data = data.decode("LATIN-1")
                    logger.info("Contents of {} as Latin-1: {}".format(filename, data))
            except Exception as e:
                logger.info(f"Failed to read file: {e}")
        else:
            logger.info(f"File {filename} does not exist")
        return False

    def dump_logs(self, *filenames):
        for filename in filenames:
            robot.libraries.BuiltIn.BuiltIn().run_keyword("LogUtils.Dump Log", filename)

    def dump_log_on_failure(self, filename):
        robot.libraries.BuiltIn.BuiltIn().run_keyword_if_test_failed("LogUtils.Dump Log", filename)

    def check_log_contains(self, string_to_contain, path_to_log, log_name=None):
        if log_name is None:
            log_name = os.path.basename(path_to_log)

        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {log_name} at location {path_to_log} does not exist ")

        contents = _get_log_contents(path_to_log)
        if string_to_contain not in contents:
            self.dump_log(path_to_log)
            raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does not contain: {string_to_contain}")

    def file_log_contains(self, path, expected):
        return self.check_log_contains(expected, path)

    def check_log_does_not_contain(self, string_not_to_contain, path_to_log, log_name):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {log_name} at location {path_to_log} does not exist ")

        contents = _get_log_contents(path_to_log)
        if string_not_to_contain in contents:
            self.dump_log(path_to_log)
            raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does contain: {string_not_to_contain}")

    def check_log_contains_string_n_times(self, log_path, log_name, string_to_contain, expected_occurrence):
        contents = _get_log_contents(log_path)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"{log_name} Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def should_contain_n_times(self, string_to_check, string_to_check_for, n):
        n = int(n)
        occurrences = self.get_number_of_occurrences_of_substring_in_string(string_to_check, string_to_check_for)
        if occurrences != n:
            logger.error(f"expected '{string_to_check}' to contain '{string_to_check_for}' {n} times, "
                         f"found {occurrences}")
            raise AssertionError()

    def check_log_contains_string_at_least_n_times(self, log_path, log_name, string_to_contain, expected_occurrence):
        contents = _get_log_contents(log_path)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences < int(expected_occurrence):
            raise AssertionError(f"{log_name} Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")
        return num_occurrences

    def check_log_and_return_nth_occurrence_between_strings(self, string_to_contain_start, string_to_contain_end,
                                                            path_to_log, occurs=1):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {path_to_log} does not exist")

        occurs_int = int(occurs)

        contents = _get_log_contents(path_to_log)

        index2 = 0
        while True:
            index1 = contents.find(string_to_contain_start, index2)
            if index1 == -1:
                break
            index2 = contents.find(string_to_contain_end, index1 + len(string_to_contain_start))
            logger.info(f"Index2 = {index2}")
            if index2 == -1:
                break
            index2 = index2 + len(string_to_contain_end)
            occurs_int -= 1
            if occurs_int == 0:
                return contents[index1:index2]

        self.dump_log(path_to_log)
        raise AssertionError(f"Log at \"{path_to_log}\" does not contain following string pair {string_to_contain_start}"
                             f" - {string_to_contain_end} times : {occurs}")

    def check_log_contains_in_order(self, log_path, *args):
        log_contains_in_order(log_path, log_path, args)

    def log_contains_in_order(self, log_location, log_name, args, log_finds=True):
        return log_contains_in_order(log_location, log_name, args, log_finds)

    def check_log_contains_more_than_one_pair_of_strings_in_order(self, string_one, string_two, path_to_log, log_name):
        logger.info(f"Strings are {string_one} {string_two}")
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {log_name} at location {path_to_log} does not exist")

        contents = _get_log_contents(path_to_log)

        first = True
        index2 = 0
        while True:
            index1 = contents.find(string_one, index2)
            logger.info(f"Index1 = {index1}")
            if index1 == -1:
                if first:
                    raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does contain first string in pair at all: "
                                         f"{string_one} - {string_two}")
                else:
                    break
            index1 = index1 + len(string_one)
            index2 = contents.find(string_two, index1)
            logger.info(f"Index2 = {index2}")
            if index2 == -1:
                self.dump_log(path_to_log)
                raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does contain pair of strings in order: "
                                     f"{string_one} - {string_two}")
            index2 = index2 + len(string_two)
            first = False

    def check_log_contains_data_from_multisend(self, log_root, subscriber, publisher_num, data_items, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            data = []
            for datum in range(0, int(data_items)):
                data.append(f"Sub {subscriber} received message: ['{channel_name}', "
                            f"'Data From publisher {publisher_id} Num {datum}']")
            log_contains_in_order(os.path.join(log_root, "fake_multi_subscriber.log"), "Fake Multi-Subscriber Log",
                                  data, False)

    def check_multiple_logs_contains_data_from_multisend(self, log_root, publisher_num, subscriber_num, data_items,
                                                         channel_name_log, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                data = []
                for datum in range(0, int(data_items)):
                    data.append(f"Sub Subscriber_{subscriber_id}_{channel_name_log} received message: "
                                f"['{channel_name}', 'Data From publisher {publisher_id} Num {datum}']")
                log_contains_in_order(
                    os.path.join(log_root, f"Subscriber_{subscriber_id}_{channel_name_log}.log"),
                    f"Fake Subscriber {subscriber_id} {channel_name_log} Log", data, False)

    def check_multiple_logs_do_not_contain_data_from_multisend(self, log_root, publisher_num, subscriber_num,
                                                               channel_name_listened_to, channel_name_not_listened_to):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                self.check_log_does_not_contain(
                    f"Sub Subscriber_{subscriber_id}_{channel_name_listened_to} received message: "
                    f"['{channel_name_not_listened_to}', 'Data From publisher {publisher_id} Num",
                    os.path.join(log_root, f"Subscriber_{subscriber_id}_{channel_name_listened_to}.log"),
                    f"Fake Subscriber {subscriber_id} {channel_name_listened_to} Log"
                )

    def mark_expected_critical_in_log(self, log_location, error_message):
        error_string = "CRITICAL"
        mark_string = "expected-critical"
        return _mark_expected_lines_in_log(log_location, error_string, mark_string, error_message)

    def mark_expected_error_in_log(self, log_location, error_message):
        self.__m_pending_mark_expected_errors.setdefault(log_location, []).append(error_message)
        return _mark_expected_errors_in_log(log_location, error_message)

    def mark_expected_fatal_in_log(self, log_location, error_message):
        error_string = "FATAL"
        mark_string = "expected-fatal"
        return _mark_expected_lines_in_log(log_location, error_string, mark_string, error_message)

    def log_string_if_found(self, string_to_contain, path_to_log):
        with open(path_to_log, "rb") as file:
            contents = file.read()
            contents = contents.decode("UTF-8", errors='backslashreplace').splitlines()
            for line in contents:
                if string_to_contain in line:
                    logger.info("{} contains: {} in {}".format(os.path.basename(path_to_log), string_to_contain, line))

    def replace_all_in_file(self, log_location, target, replacement):
        contents = _get_log_contents(log_location)
        contents = contents.replace(target, replacement)
        with open(log_location, "w") as log:
            log.write(contents)

    def check_all_product_logs_do_not_contain_string(self, string_to_find, search_list=None):
        if search_list is None:
            search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*", "plugins/av/log/sophos_threat_detector/*.log*"]
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combined_files = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        flat_files = [item for sublist in combined_files for item in sublist]

        list_of_logs_containing_string = []
        for filepath in flat_files:
            try:
                num_occurrence = self.get_number_of_occurrences_of_substring_in_log(filepath, string_to_find)
            except Exception as ex:
                logger.error("Failed to process {}: {}".format(filepath, ex))
                robot.libraries.BuiltIn.BuiltIn().run_keyword("LogUtils.Dump Log", filepath)
                raise

            if num_occurrence > 0:
                self.log_string_if_found(string_to_find, filepath)
                list_of_logs_containing_string.append(f"{filepath} - {num_occurrence} times")
                robot.libraries.BuiltIn.BuiltIn().run_keyword("LogUtils.Dump Log", filepath)
                # Edit file to avoid cascading failures
                replacement = "KNOWN" + "!" * len(string_to_find)
                replacement = replacement[:len(string_to_find)]  # same length as string_to_find
                self.replace_all_in_file(filepath, string_to_find, replacement)
        if list_of_logs_containing_string:
            raise AssertionError(f"These program logs contain {string_to_find}:\n {list_of_logs_containing_string}")

    def check_all_product_logs_do_not_contain_critical(self):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*", "plugins/av/log/sophos_threat_detector/*.log*"]
        self.check_all_product_logs_do_not_contain_string("CRITICAL", search_list)

    def check_all_product_logs_do_not_contain_fatal(self):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*", "plugins/av/log/sophos_threat_detector/*.log*"]
        self.check_all_product_logs_do_not_contain_string("FATAL", search_list)

    def check_logs_do_not_contain_error(self, log_file_list):
        logger.info("Re-apply expected errors")
        for log_location, error_messages in self.__m_pending_mark_expected_errors.items():
            _mark_expected_errors_in_log(log_location, *error_messages)
        self.__m_pending_mark_expected_errors = {}

        self.check_all_product_logs_do_not_contain_string("]   ERROR [", log_file_list)

    def check_all_product_logs_do_not_contain_error(self):
        log_file_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*",
                       "plugins/av/log/sophos_threat_detector/sophos_threat_detector*.log*"]
        self.check_logs_do_not_contain_error(log_file_list)

    def verify_message_relay_failure_in_order(self, *messagerelays, **kwargs):
        mcs_address = kwargs.get("MCS_ADDRESS", "mcs.sandbox.sophos:443")
        targets = []
        for mr in messagerelays:
            targets.append(f"Failed connection with message relay via {mr} to {mcs_address}:")

        log_contains_in_order(self.mcs_router_log, "mcs_router", targets)

    # Mainly for debugging; this function will add a marker line to all log files so that we can easily view logs
    # from each part of the tests, a custom tag can also be included.
    def mark_all_logs(self, tag=""):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*"]
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combined_files = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        log_files = [item for sublist in combined_files for item in sublist]
        for filepath in log_files:
            with open(filepath, 'a') as log_file:
                log_file.write(f'-{tag}----------------------------------------------------------\n')

    # Cloud Server Log Utils
    def dump_cloud_server_log(self):
        self.dump_log(self.cloud_server_log)

    def mark_cloud_server_log(self):
        return self.mark_log_size(self.cloud_server_log)

    def cloud_server_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.cloud_server_log, "cloud server log")

    def cloud_server_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.cloud_server_log, "cloud server log")

    # ThinInstaller Log Utils
    def dump_thininstaller_log(self):
        available = self.dump_log(self.thin_install_log)
        # Check for sync log
        if not available:
            return

        try:
            with open(self.thin_install_log) as f:
                contents = f.read()
        except EnvironmentError:
            logger.info("Can't read contents if thin installer log")

        matcher = re.compile(r"Copying SPL logs to (/tmp/SophosCentralInstall_[a-zA-Z0-9]+/logs)")
        mo = matcher.search(contents)
        if not mo:
            logger.info("Failed to find installer tmp dir")
            return

        log_dir = mo.group(1)
        logs = os.listdir(log_dir)
        logger.info("Logs: %s" % str(logs))

        for base, dirs, files in os.walk(log_dir):
            logger.info("Logs: %s in %s" % (str(files), base))
            for log in files:
                self.dump_log(os.path.join(base, log))

    def remove_thininstaller_log(self):
        if os.path.isfile(self.thin_install_log):
            os.remove(self.thin_install_log)

    def check_thininstaller_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_contains_pattern(self, pattern_to_contain):
        self.check_string_matching_regex_in_file(self.thin_install_log, pattern_to_contain)

    def check_thininstaller_log_does_not_contain(self, string_not_to_contain):
        self.check_log_does_not_contain(string_not_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_contains_in_order(self, *args):
        log_contains_in_order(self.thin_install_log, "Thin installer", args)

    def mark_expected_error_in_thininstaller_log(self, error_message):
        self.mark_expected_error_in_log(self.thin_install_log, error_message)

    # Register Central Log Utils
    def get_register_central_log(self):
        return self.register_log

    def dump_register_central_log(self):
        self.dump_log(self.register_log)

    def check_register_central_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.register_log, "Register Central")

    def check_register_central_log_does_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.register_log, "Register Central")

    def check_register_central_log_contains_in_order(self, *args):
        log_contains_in_order(self.register_log, "Register Central", args)

    # SulDownloader Log Utils
    def get_suldownloader_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.suldownloader_log)

    def check_suldownloader_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.suldownloader_log, "Suldownloader")

    def check_suldownloader_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.suldownloader_log, "Suldownloader")

    def check_suldownloader_log_contains_in_order(self, *args):
        log_contains_in_order(self.suldownloader_log, "Suldownloader", args)

    def check_suldownloader_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.suldownloader_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain,
                                                                               use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"suldownloader Log Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def mark_sul_log(self):
        self.marked_sul_logs = get_log_length(self.suldownloader_log)

    def check_marked_sul_log_contains(self, string_to_contain):
        sul_log = self.suldownloader_log
        contents = _get_log_contents(sul_log)

        contents = contents[self.marked_sul_logs:]

        if string_to_contain not in contents:
            self.dump_log(sul_log)
            raise AssertionError(f"SUL downloader log did not contain: {string_to_contain}")

    def check_marked_sul_log_does_not_contain(self, string_to_not_contain):
        sul_log = self.suldownloader_log
        contents = _get_log_contents(sul_log)

        contents = contents[self.marked_sul_logs:]

        if string_to_not_contain in contents:
            self.dump_log(sul_log)
            raise AssertionError(f"SUL downloader log contains: {string_to_not_contain}")

    # Watchdog Log Utils
    def dump_watchdog_log(self):
        self.dump_log(self.watchdog_log)

    def mark_watchdog_log(self):
        self.marked_watchdog_logs = get_log_length(self.watchdog_log)

    def check_watchdog_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.watchdog_log, "Watchdog")
        logger.info(self.watchdog_log)

    def check_watchdog_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.watchdog_log, "Watchdog")

    def check_marked_watchdog_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain not in contents:
            self.dump_watchdog_log()
            raise AssertionError(f"Marked watchdog log did not contain: {string_to_contain}")

    def check_marked_watchdog_log_does_not_contain(self, string_to_contain):
        contents = _get_log_contents(self.watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain in contents:
            self.dump_watchdog_log()
            raise AssertionError(f"Marked watchdog log did not contain: {string_to_contain}")

    # MCS Router Log Utils
    def dump_mcs_config(self):
        config = os.path.join(self.install_path, "base", "etc", "mcs.config")
        sophosav_config = os.path.join(self.install_path, "base", "etc", "sophosspl", "mcs.config")
        for c in [config, sophosav_config]:
            logger.info(c)
            self.dump_log(c)

    def dump_mcs_policy_config(self):
        config = os.path.join(self.install_path, "base", "etc", "mcs_policy.config")
        sophosav_config = os.path.join(self.install_path, "base", "etc", "sophosspl", "mcs_policy.config")
        for c in [config, sophosav_config]:
            logger.info(c)
            self.dump_log(c)

    def dump_all_mcs_events(self):
        mcs_events_dir = os.path.join(self.install_path, "base/mcs/event")
        for filename in os.listdir(mcs_events_dir):
            fullpath = os.path.join(mcs_events_dir, filename)
            self.dump_log(fullpath)

    def dump_mcs_envelope_log(self):
        self.dump_log(self.mcs_envelope_log)

    def check_mcsenvelope_log_contains(self, string_to_contain):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.check_log_contains(string_to_contain, mcs_envelope_log, "MCS Envelope")

    def check_mcsenvelope_log_does_not_contain(self, string_to_not_contain):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.check_log_does_not_contain(string_to_not_contain, mcs_envelope_log, "MCS Envelope")

    def check_mcsenvelope_log_contains_in_order(self, *args):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        log_contains_in_order(mcs_envelope_log, "MCS Envelope", args)

    def check_mcs_envelope_log_contains_string_n_times(self, string_to_contain, expected_occurrence, use_regex=False):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        contents = _get_log_contents(mcs_envelope_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"mcs_envelope Log Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def check_mcs_envelope_log_contains_regex_string_n_times(self, string_to_contain, expected_occurrence):
        self.check_mcs_envelope_log_contains_string_n_times(string_to_contain, expected_occurrence, True)

    def check_mcs_envelope_for_status_same_in_at_least_one_of_the_following_statuses_sent_with_the_prior_being_noref(self, *status_numbers):
        # status_numbers should be integers
        # will check all statuses in order given to see if they are res=same
        # will pass when res=same is found
        # will fail on a status where res is neither same nor noref
        # the purpose of this function is to solve a race condition in a flakey test
        statuses = []
        for status_number in status_numbers:
            try:
                statuses.append(self.check_log_and_return_nth_occurrence_between_strings(
                    "<status><appId>ALC</appId>",
                    "</status>",
                    f"{self.install_path}/logs/base/sophosspl/mcs_envelope.log",
                    int(status_number))
                )
            except AssertionError:
                logger.info(f"Failed to get string for status {status_number}")

        status_noref = "Res=&amp;quot;NoRef&amp;quot;"
        status_same = "Res=&amp;quot;Same&amp;quot;"
        count = 0
        for status in statuses:
            if status_same in status:
                logger.debug("got status: same")
                return int(status_numbers[count])
            elif status_noref not in status:
                logger.debug("got unexpected status")
                raise AssertionError(f"Status: {status}\n was neither {status_noref} or {status_same}")
            count += 1
        raise AssertionError(f"Failed to find {status_same} in statuses: {status_numbers}")

    def mark_mcs_envelope_log(self):
        self.marked_mcs_envelope_logs = get_log_length(self.mcs_envelope_log)

    def check_marked_mcs_envelope_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.mcs_envelope_log)

        contents = contents[self.marked_mcs_envelope_logs:]

        if string_to_contain not in contents:
            self.dump_mcs_envelope_log()
            raise AssertionError(f"MCS Envelope log did not contain: {string_to_contain}")

    def dump_mcsrouter_log(self):
        self.dump_log(self.mcs_router_log)

    def get_mark_for_mcsrouter_log(self):
        return self.mark_log_size(self.mcs_router_log)

    def check_mcsrouter_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.mcs_router_log, "MCS Router")
        logger.info(self.mcs_router_log)

    def check_mcsrouter_log_contains_pattern(self, pattern_to_contain):
        self.check_string_matching_regex_in_file(self.mcs_router_log, pattern_to_contain)

    def check_mcsrouter_log_contains_in_order(self, *args):
        log_contains_in_order(self.mcs_router_log, "MCS Router", args)

    def check_mcsrouter_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.mcs_router_log, "MCS Router")

    def check_mcsrouter_does_not_contain_critical_exceptions(self):
        if os.path.exists(self.mcs_router_log):
            self.check_log_does_not_contain("Caught exception at top-level;", self.mcs_router_log, "MCS router")

    def mark_mcsrouter_log(self):
        self.marked_mcsrouter_logs = get_log_length(self.mcs_router_log)
        logger.info(f"mcsrouter log marked at char: {self.marked_mcsrouter_logs}")

    def check_marked_mcsrouter_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.mcs_router_log)

        contents = contents[self.marked_mcsrouter_logs:]

        if string_to_contain not in contents:
            self.dump_mcsrouter_log()
            raise AssertionError(f"MCS Router log did not contain: {string_to_contain}")

    def check_marked_mcsrouter_log_does_not_contain(self, string_not_to_contain):
        contents = _get_log_contents(self.mcs_router_log)
        contents = contents[self.marked_mcsrouter_logs:]

        if string_not_to_contain in contents:
            self.dump_mcsrouter_log()
            raise AssertionError(f"MCS Router log contained: {string_not_to_contain}")

    def check_marked_mcsrouter_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.mcs_router_log)

        contents = contents[self.marked_mcsrouter_logs:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                f"McsRouter Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                f"{expected_occurrence} times")

    # Management Agent Log Utils
    def dump_managementagent_log(self):
        self.dump_log(self.managementagent_log)

    def mark_managementagent_log(self) -> LogHandler.LogMark:
        self.marked_managementagent_log = get_log_length(self.managementagent_log)
        return LogHandler.LogMark(self.managementagent_log)

    def check_management_agent_log_contains(self, string_to_contain):
        log = self.managementagent_log
        self.check_log_contains(string_to_contain, log, "ManagementAgent")
        logger.info(log)

    def check_marked_managementagent_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.managementagent_log)

        contents = contents[self.marked_managementagent_log:]

        if string_to_contain not in contents:
            self.dump_managementagent_log()
            raise AssertionError(f"Marked managementagent log did not contain: {string_to_contain}")

    def check_managementagent_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.managementagent_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"managementagent Log Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def check_marked_managementagent_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.managementagent_log)

        contents = contents[self.marked_managementagent_log:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                f"Marked Management Agent Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the "
                f"requested {expected_occurrence} times")

    def does_management_agent_log_contain(self, string_to_contain):
        try:
            self.check_management_agent_log_contains(string_to_contain)
            return True
        except AssertionError:
            return False

    # Telemetry Scheduler Log Utils
    def check_tscheduler_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.tscheduler_log, "Tscheduler")

    # Update Scheduler Log Utils
    def check_updatescheduler_log_contains(self, string_to_contain):
        updatescheduler_log = self.update_scheduler_log
        self.check_log_contains(string_to_contain, updatescheduler_log, "UpdateScheduler")
        logger.info(updatescheduler_log)

    def check_updatescheduler_log_does_not_contain(self, string_to_not_contain):
        log = self.update_scheduler_log
        self.check_log_does_not_contain(string_to_not_contain, log, "Updatescheduler")

    def check_updatescheduler_log_contains_in_order(self, *args):
        log_contains_in_order(self.update_scheduler_log, "UpdateScheduler", args)

    def check_updatescheduler_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.update_scheduler_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"updatescheduler Log Contains: \"{string_to_contain}\" - {num_occurrences} times not "
                                 f"the requested {expected_occurrence} times")

    def mark_update_scheduler_log(self):
        self.marked_update_scheduler_logs = get_log_length(self.update_scheduler_log)
        return self.mark_log_size(self.update_scheduler_log)

    def check_marked_update_scheduler_log_contains(self, string_to_contain):
        update_scheduler_log = self.update_scheduler_log
        contents = _get_log_contents(update_scheduler_log)

        contents = contents[self.marked_update_scheduler_logs:]

        if string_to_contain not in contents:
            self.dump_log(update_scheduler_log)
            raise AssertionError(f"Marked update scheduler log did not contain: {string_to_contain}")

    # AV Plugin Log Utils
    def mark_av_log(self):
        self.marked_av_log = self.mark_log_size(self.av_log)
        return self.marked_av_log

    def check_marked_av_log_contains(self, string_to_contain):
        av_log = self.av_log
        contents = _get_log_contents(av_log)

        contents = contents[self.marked_av_log:]

        if string_to_contain not in contents:
            self.dump_log(av_log)
            raise AssertionError(f"av.log log did not contain: {string_to_contain}")

    def check_safestore_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.safestore_log, "SafeStore")

    def check_safestore_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.safestore_log, "SafeStore")

    def mark_safestore_log(self):
        self.marked_safestore_log = self.mark_log_size(self.safestore_log)
        return self.marked_safestore_log

    def check_marked_safestore_log_contains(self, string_to_contain):
        safestore_log = self.safestore_log
        contents = _get_log_contents(safestore_log)

        contents = contents[self.marked_safestore_log:]

        if string_to_contain not in contents:
            self.dump_log(safestore_log)
            raise AssertionError(f"SafeStore log did not contain: {string_to_contain}")

    def mark_sophos_threat_detector_log(self) -> LogHandler.LogMark:
        self.marked_sophos_threat_detector_log = self.mark_log_size(self.sophos_threat_detector_log)
        return self.marked_sophos_threat_detector_log

    def check_marked_sophos_threat_detector_log_contains(self, string_to_contain, mark=None):
        sophos_threat_detector_log = self.sophos_threat_detector_log

        if mark is None:
            if isinstance(self.marked_sophos_threat_detector_log, LogHandler.LogMark):
                contents = self.marked_sophos_threat_detector_log.get_contents_unicode()
            else:
                contents = _get_log_contents(sophos_threat_detector_log)
        else:
            contents = mark.get_contents_unicode()

        if string_to_contain not in contents:
            self.dump_log(sophos_threat_detector_log)
            raise AssertionError(f"sophos_threat_detector.log log did not contain: {string_to_contain}")

        return contents

    def mark_susi_debug_log(self) -> LogHandler.LogMark:
        self.marked_susi_debug_log = self.mark_log_size(self.susi_debug_log)
        return self.marked_susi_debug_log

    def mark_scheduled_scan_log(self) -> LogHandler.LogMark:
        self.marked_ss_log = self.mark_log_size(self.scheduled_scan_log)
        return self.marked_ss_log

    def mark_scan_now_log(self) -> LogHandler.LogMark:
        self.marked_scan_now_log = self.mark_log_size(self.scan_now_log)
        return self.marked_scan_now_log

    # EDR Log Utils
    def check_edr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.edr_log, "EDR")

    def check_edr_log_does_not_contain(self, string_to_not_contain):
        log = self.edr_log
        self.check_log_does_not_contain(string_to_not_contain, log, "EDR")

    def check_edr_log_contains_string_n_times(self, string_to_contain, expected_occurrence):

        log = self.edr_log
        contents = _get_log_contents(log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"EDR Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                                 f"{expected_occurrence} times")

    def mark_edr_log(self):
        self.marked_edr_log = get_log_length(self.edr_log)

    def check_marked_edr_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.edr_log)

        contents = contents[self.marked_edr_log:]

        if string_to_contain not in contents:
            raise AssertionError(f"EDR did not contain: {string_to_contain}")

    def mark_edr_osquery_log(self):
        self.marked_edr_osquery_log = get_log_length(self.edr_osquery_log)

    def check_marked_edr_osquery_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.edr_osquery_log)

        contents = contents[self.marked_edr_osquery_log:]

        if string_to_contain not in contents:
            raise AssertionError(f"EDR did not contain: {string_to_contain}")

    def dump_livequery_log(self):
        self.dump_log(self.livequery_log)

    def check_livequery_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.livequery_log, "Livequery")

    def mark_livequery_log(self):
        self.marked_livequery_log = get_log_length(self.livequery_log)
        logger.info(f"livequery log marked at char: {self.marked_livequery_log}")

    def check_marked_livequery_log_contains(self, string_to_contain):
        contents = _get_log_contents(self.livequery_log)

        contents = contents[self.marked_livequery_log:]

        if string_to_contain not in contents:
            self.dump_livequery_log()
            raise AssertionError(f"Marked livequery log did not contain: {string_to_contain}")

    def check_marked_livequery_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = _get_log_contents(self.livequery_log)

        contents = contents[self.marked_livequery_log:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                f"Livequery Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                f"{expected_occurrence} times")

    # Event Journaler Log Utils
    def get_event_journaler_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.ej_log)

    def check_event_journaler_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.ej_log, "Journaler")

    # Device Isolation Log Utils
    def get_device_isolation_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.deviceisolation_log)

    def check_device_isolation_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.deviceisolation_log, "Device Isolation")

    # Live Response Log Utils
    def check_liveresponse_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.liveresponse_log, "Liveresponse")

    def check_sessions_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.sessions_log, "sessions")

    def dump_push_server_log(self):
        server_log = os.path.join(self.tmp_path, "push_server.log")
        self.dump_log(server_log)

    def dump_warehouse_log(self):
        server_log = os.path.join(self.tmp_path, "warehouseGenerator.log")
        self.dump_log(server_log)

    def dump_proxy_logs(self):
        message_relay_log = os.path.join(self.tmp_path, "relay.log")
        proxy_log = os.path.join(self.tmp_path, "proxy.log")
        update_proxy_log = os.path.join(self.tmp_path, "proxy_server.log")
        for log in [message_relay_log, proxy_log, update_proxy_log]:
            if os.path.isfile(log):
                self.dump_log(log)

########################################################################################################################
# Log Handler

    def get_log_handler(self, logpath) -> LogHandler.LogHandler:
        handler = self.__m_log_handlers.get(logpath, None)
        if handler is None:
            rtd = logpath == self.rtd_log
            handler = LogHandler.LogHandler(logpath, rtd=rtd)
            self.__m_log_handlers[logpath] = handler
        return handler

    def mark_log_size(self, logpath: str) -> LogHandler.LogMark:
        handler = self.get_log_handler(logpath)
        mark = handler.get_mark()
        self.__m_marked_log_position[logpath] = mark  # Save the most recent marked position
        return mark

    def mark_all_plugin_logs(self, install_path=None) -> dict:
        if install_path is not None:
            self.__set_log_paths(install_path)

        plugin_log_marks = {
            # AV
            "sophos_threat_detector_mark": self.mark_log_size(self.sophos_threat_detector_log),
            "susi_debug_mark": self.mark_log_size(self.susi_debug_log),
            "av_mark": self.mark_log_size(self.av_log),
            "oa_mark": self.mark_log_size(self.oa_log),
            "safestore_mark": self.mark_log_size(self.safestore_log),
            "scheduled_scan_mark": self.mark_log_size(self.scheduled_scan_log),
            "scan_now_mark": self.mark_log_size(self.scan_now_log),

            # Base
            "watchdog_mark": self.mark_log_size(self.watchdog_log),
            "wdctl_mark": self.mark_log_size(self.wdctl_log),
            "managementagent_mark": self.mark_log_size(self.managementagent_log),
            "mcs_router_mark": self.mark_log_size(self.mcs_router_log),
            "mcs_envelop_mark": self.mark_log_size(self.mcs_envelope_log),
            "telemetry_mark": self.mark_log_size(self.telemetry_log),
            "tscheduler_mark": self.mark_log_size(self.tscheduler_log),
            "update_scheduler_mark": self.mark_log_size(self.update_scheduler_log),
            "diagnose_mark": self.mark_log_size(self.diagnose_log),
            "sdu_mark": self.mark_log_size(self.sdu_log),

            # RA
            "response_actions_mark": self.mark_log_size(self.responseactions_log),

            # DI
            "deviceisolation_mark": self.mark_log_size(self.deviceisolation_log),

            # EDR
            "edr_mark": self.mark_log_size(self.edr_log),
            "edr_osquery_mark": self.mark_log_size(self.edr_osquery_log),
            "scheduled_query_mark": self.mark_log_size(self.scheduled_query_log),

            # EJ
            "ej_mark": self.mark_log_size(self.ej_log),

            # LR
            "liveresponse_mark": self.mark_log_size(self.liveresponse_log),
            "sessions_mark": self.mark_log_size(self.sessions_log),

            # RTD
            "rtd_mark": self.mark_log_size(self.rtd_log)
        }

        return plugin_log_marks

    def wait_for_log_contains_from_mark(self,
                                        mark: LogHandler.LogMark,
                                        expected: typing.Union[list, str, bytes],
                                        timeout=10) -> LogHandler.LogMark:
        assert mark is not None
        assert expected is not None
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_from_mark"
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def wait_for_possible_log_contains_from_mark(self,
                                                 mark: LogHandler.LogMark,
                                                 expected: typing.Union[list, str, bytes],
                                                 timeout: float = 10) -> LogHandler.LogMark:
        assert mark is not None
        assert expected is not None
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_possible_log_contains_from_mark"
        return mark.wait_for_possible_log_contains_from_mark(expected, timeout)

    def wait_for_log_contains_after_mark(self,
                                         logpath: typing.Union[str, bytes],
                                         expected: typing.Union[list, str, bytes],
                                         mark: LogHandler.LogMark,
                                         timeout=10) -> LogHandler.LogMark:
        if mark is None:
            logger.error("No mark passed for wait_for_log_contains_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected, logpath))
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_after_mark"
        mark.assert_paths_match(logpath)
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def check_log_contains_after_mark(self, log_path, expected, mark):
        if mark is None:
            logger.error("No mark passed for check_log_contains_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected, log_path))

        if isinstance(expected, str):
            expected = expected.encode("UTF-8")

        mark.assert_paths_match(log_path)
        contents = mark.get_contents()
        if expected in contents:
            return

        logger.error("Failed to find %s in %s" % (expected, log_path))
        handler = self.get_log_handler(log_path)
        handler.dump_marked_log(mark)
        raise AssertionError("Failed to find %s in %s" % (expected, log_path))

    def get_log_after_mark(self, log_path, mark):
        assert mark is not None
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in get_log_after_mark"
        handler = self.get_log_handler(log_path)
        return handler.get_contents(mark)

    def dump_marked_log(self, log_path: str, mark=None):
        if mark is None:
            mark = self.__m_marked_log_position[log_path]
        handler = self.get_log_handler(log_path)
        handler.dump_marked_log(mark)

    def dump_from_mark(self, mark: LogHandler.LogMark):
        mark.dump_marked_log()

    def check_log_does_not_contain_after_mark(self, log_path, not_expected, mark: LogHandler.LogMark) -> None:
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in check_log_does_not_contain_after_mark"
        not_expected = LogHandler.ensure_binary(not_expected)
        mark.assert_paths_match(log_path)
        contents = mark.get_contents()
        if not_expected in contents:
            self.dump_marked_log(log_path, mark)
            raise AssertionError("Found %s in %s" % (not_expected, log_path))

    def wait_for_log_to_not_contain_after_mark(self, log_path, not_expected, mark: LogHandler.LogMark, timeout: int):
        """Wait for timeout and report if the log does contain not_expected
        """
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_to_not_contain_after_mark"
        mark.assert_paths_match(log_path)
        time.sleep(timeout)
        return self.check_log_does_not_contain_after_mark(log_path, not_expected, mark)

    def wait_for_log_to_not_contain_from_start_of_file(self, log_path: str, not_expected: str, timeout: int):
        mark = LogHandler.LogMark(log_path, 0)
        return self.wait_for_log_to_not_contain_after_mark(log_path, not_expected, mark, timeout)

    def wait_for_log_contains_after_last_restart(self, log_path, expected, timeout: int = 10, mark=None):
        """
        Wait for a log line, but only in the log lines after the most recent restart of the process.

        :param log_path:
        :param expected:
        :param timeout:
        :param mark: Also restrict log lines after the mark
        :return:
        """
        handler = self.get_log_handler(log_path)
        return handler.Wait_For_Log_contains_after_last_restart(expected, timeout, mark)

    def check_log_contains_n_times_after_mark(self, log_path, expected_log, times, mark):
        expected_list = []
        for x in range(0, int(times)):
            expected_list.append(expected_log)

        self.check_log_contains_in_order_after_mark(log_path, expected_list, mark)

    def check_log_contains_in_order_after_mark(self, log_path, expected_items, mark):
        if mark is None:
            logger.error("No mark passed for check_log_contains_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected_items, log_path))

        encoded_expected = []
        for string in expected_items:
            encoded_expected.append(string.encode("UTF-8"))

        mark.assert_is_good(log_path)
        contents = mark.get_contents()
        index = 0

        for string in encoded_expected:
            logger.info("Looking for {}".format(string))
            index = contents.find(string, index)
            if index != -1:
                logger.info("{} log contains {}".format(log_path, string))
                index = index + len(string)
            else:
                logger.error(contents)
                raise AssertionError("Remainder of {} log doesn't contain {}".format(log_path, string))

        return

    def wait_for_log_contains_n_times_from_mark(self,
                                                mark: LogHandler.LogMark,
                                                expected: typing.Union[list, str, bytes],
                                                times: int,
                                                timeout=10) -> None:
        assert mark is not None
        assert expected is not None
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_from_mark"
        return mark.wait_for_log_contains_n_times_from_mark(expected, times, timeout)

    def wait_for_log_contains_one_of_after_mark(self,
                                                logpath: typing.Union[str, bytes],
                                                expected: typing.Union[list, str, bytes],
                                                mark: LogHandler.LogMark,
                                                timeout=10) -> None:
        if mark is None:
            logger.error("No mark passed for wait_for_log_contains_one_of_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected, logpath))
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_one_of_after_mark"
        mark.assert_is_good(logpath)
        return mark.wait_for_log_contains_one_of_from_mark(expected, timeout)

    def wait_for_log_contains_any(self,
                                  mark: LogHandler.LogMark,
                                  *expected,
                                  timeout=10) -> LogHandler.LogMark:
        assert mark is not None
        assert expected is not None
        assert isinstance(expected, tuple)
        assert isinstance(mark, LogHandler.LogMark), ("mark is not an instance of LogMark in "
                                                      "wait_for_log_contains_from_mark")
        return mark.wait_for_log_contains_from_mark(list(expected), timeout)

    def find_last_match_after_mark(self, mark, line_to_search_for):
        return mark.find_last_match_after_mark(line_to_search_for)

    def save_log_marks_at_start_of_test(self):
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.mark_log_size(self.oa_log))
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${AV_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.mark_log_size(self.av_log))
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${SAFESTORE_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.mark_log_size(self.safestore_log))
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${THREATDETECTOR_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.mark_log_size(self.sophos_threat_detector_log))

########################################################################################################################
# RTD Log
    def wait_for_rtd_log_contains_after_last_restart(self, log_path, expected, timeout: int = 20, mark=None):
        handler = self.get_log_handler(log_path)
        return handler.Wait_For_Log_contains_after_last_restart(expected, timeout, mark)

########################################################################################################################
# On-Access Soapd Log
    def get_on_access_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.oa_log)

    def get_on_access_log_mark_if_required(self, mark) -> LogHandler.LogMark:
        """
        Get a mark if the argument mark is None
        """
        if mark is not None:
            return mark
        return self.mark_log_size(self.oa_log)

    def save_on_access_log_mark_at_start_of_test(self):
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.get_on_access_log_mark())

    def dump_on_access_log_after_mark(self, mark: LogHandler.LogMark):
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in dump_on_access_log_after_mark"
        self.dump_marked_log(self.oa_log, mark)

    def wait_for_on_access_log_contains_after_mark(self, expected, mark: LogHandler.LogMark, timeout: int = 10):
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_on_access_log_contains_after_mark"
        return self.wait_for_log_contains_after_mark(self.oa_log, expected, mark, timeout=timeout)

    def check_on_access_log_contains_after_mark(self, expected, mark):
        return self.check_log_contains_after_mark(self.oa_log, expected, mark)

    def check_on_access_log_does_not_contain_after_mark(self, notexpected, mark):
        return self.check_log_does_not_contain_after_mark(self.oa_log, notexpected, mark)

    def check_on_access_log_does_not_contain_before_timeout(self, not_expected, mark, timeout: int = 5):
        """Wait for timeout and report if the log does contain notexpected
        """
        time.sleep(timeout)
        return self.check_on_access_log_does_not_contain_after_mark(not_expected, mark)

    def check_strace_log_for_openssl_mentions(self, logpath):
        with open(logpath, "r") as f:
            contents = f.read()
            path_regex = re.compile(r"\"(.+openssl\.cnf)\"")
            matches = path_regex.findall(contents)
            exclusions = [
                "/etc/ssl/openssl.cnf",
            ]
            unexpected_matches = [match for match in matches if match not in exclusions]
            if len(unexpected_matches) > 0:
                raise AssertionError(f"Found unexpected reads of openssl configs:\n" + "\n".join(unexpected_matches))

########################################################################################################################
# AV Logs
    def wait_for_av_log_contains_after_mark(self, expected: str, mark: LogHandler.LogMark, timeout: int = 10):
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_av_log_contains_after_mark"
        return self.wait_for_log_contains_after_mark(self.av_log, expected, mark, timeout=timeout)

    def wait_for_av_log_contains_after_last_restart(self, expected, timeout: int = 15, mark=None):
        return self.wait_for_log_contains_after_last_restart(self.av_log, expected, timeout, mark)

    def wait_for_sophos_threat_detector_log_contains_after_last_restart(self, expected, timeout: int = 15, mark=None):
        return self.wait_for_log_contains_after_last_restart(self.sophos_threat_detector_log, expected, timeout, mark)

    def check_av_log_contains_after_mark(self, expected: str, mark: LogHandler.LogMark):
        return self.check_log_contains_after_mark(self.av_log, expected, mark)

    def check_av_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.av_log, not_expected, mark)

    def wait_for_sophos_threat_detector_log_contains_after_mark(self, expected: str, mark: LogHandler.LogMark, timeout: int = 10):
        return self.wait_for_log_contains_after_mark(self.sophos_threat_detector_log, expected, mark, timeout=timeout)

    def wait_for_sophos_threat_detector_log_contains_one_of_after_mark(self, expected: list, mark: LogHandler.LogMark, timeout: int = 10):
        return self.wait_for_log_contains_one_of_after_mark(self.sophos_threat_detector_log, expected, mark, timeout=timeout)

    def wait_until_latest_sophos_threat_detector_log_contains(self, expected: str, timeout: int = 15):
        handler = self.get_log_handler(self.sophos_threat_detector_log)
        mark = handler.get_mark_at_start_of_current_file()
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def check_sophos_threat_detector_log_contains_after_mark(self, expected, mark):
        return self.check_log_contains_after_mark(self.sophos_threat_detector_log, expected, mark)

    def check_sophos_threat_detector_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.sophos_threat_detector_log, not_expected, mark)

    def av_log_contains_multiple_times_after_mark(self, expected: str, mark: LogHandler.LogMark, times):
        return self.check_log_contains_n_times_after_mark(self.av_log, expected, times, mark)

    def check_susi_debug_log_contains_after_mark(self, expected, mark):
        return self.check_log_contains_after_mark(self.susi_debug_log, expected, mark)

    def check_susi_debug_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.susi_debug_log, not_expected, mark)

    def get_susi_debug_log_after_mark(self, mark):
        return self.get_log_after_mark(self.susi_debug_log, mark)

    def wait_for_susi_debug_log_contains_after_mark(self, expected: str, mark: LogHandler.LogMark, timeout: int = 10):
        return self.wait_for_log_contains_after_mark(self.susi_debug_log, expected, mark, timeout)

    def get_av_log_after_mark_as_unicode(self, mark):
        return _ensure_str(self.get_log_after_mark(self.av_log, mark))

    def check_on_access_log_contains_in_order(self, expected_items, mark):
        logger.info(expected_items)
        return self.check_log_contains_in_order_after_mark(self.oa_log, expected_items, mark)

    def check_scan_now_log_contains_after_mark(self, expected, mark):
        return self.check_log_contains_after_mark(self.scan_now_log, expected, mark)

    def check_scan_now_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.scan_now_log, not_expected, mark)

    def wait_for_safestore_log_contains_after_mark(self, expected, mark: LogHandler.LogMark, timeout: int = 10):
        return self.wait_for_log_contains_after_mark(self.safestore_log, expected, mark, timeout)

    def verify_sophos_threat_detector_log_line_is_level(self, expected_level, string_to_check, mark=None):
        contents = self.check_marked_sophos_threat_detector_log_contains(string_to_check, mark)
        line_re = re.compile(r"^\d+\s+\[\S+]\s+(\w+)\s+.*?"+re.escape(string_to_check)+r".*?$", flags=re.MULTILINE)
        found = False
        for mo in line_re.finditer(contents):
            found = True
            logger.info("Found line: %s" % mo.group(0))
            level = mo.group(1)
            logger.info("Level: %s" % level)
            if level not in expected_level:
                raise AssertionError(f"sophos_threat_detector log contains expected string \"{string_to_check}\" "
                                     f"at unexpected level {level} (expected {expected_level})")
        if not found:
            raise AssertionError(f"Regex failed to find {string_to_check}")

    def verify_sophos_threat_detector_log_line_is_informational(self, string_to_check):
        return self.verify_sophos_threat_detector_log_line_is_level(("DEBUG", "INFO", "SUPPORT", "SPRT"),
                                                                    string_to_check)

    def dump_sophos_threat_detector_log_after_mark(self, mark):
        return self.dump_marked_log(self.sophos_threat_detector_log, mark)

########################################################################################################################
# Plugin readiness checks
    def wait_for_response_action_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        response_actions_mark = log_marks["response_actions_mark"]
        # TODO: once this log line is in current shipping, remove oldcode if statement
        if not oldcode:
            response_actions_mark.wait_for_log_contains_from_mark("Completed initialization of Response Actions", timeout)
        else:
            response_actions_mark.wait_for_log_contains_from_mark("Entering the main loop", timeout)

    def wait_for_base_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        watchdog_mark = log_marks["watchdog_mark"]
        managementagent_mark = log_marks["managementagent_mark"]
        update_scheduler_mark = log_marks["update_scheduler_mark"]
        mcs_router_mark = log_marks["mcs_router_mark"]
        tscheduler_mark = log_marks["tscheduler_mark"]

        if not oldcode:
            # TODO: once this log line is in current shipping, remove oldcode if statement
            watchdog_mark.wait_for_log_contains_from_mark("Completed initialization of Watchdog", timeout)
            update_scheduler_mark.wait_for_log_contains_from_mark("Completed initialization of Update Scheduler", timeout)
        else:
            watchdog_mark.wait_for_log_contains_from_mark("Calling poller at", timeout)
            update_scheduler_mark.wait_for_log_contains_from_mark("Update Scheduler Starting", timeout)


        managementagent_mark.wait_for_log_contains_from_mark("Management Agent running", timeout)
        mcs_router_mark.wait_for_log_contains_from_mark("Started with install directory set to", timeout)
        tscheduler_mark.wait_for_log_contains_from_mark("Waiting for ALC policy before running Telemetry", timeout)

    def wait_for_edr_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30):
        edr_mark = log_marks["edr_mark"]
        edr_osquery_mark = log_marks["edr_osquery_mark"]

        edr_mark.wait_for_log_contains_from_mark("Completed initialisation of EDR", timeout)
        edr_osquery_mark.wait_for_log_contains_from_mark("osquery initialized", timeout)

    def wait_for_ej_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        ej_mark = log_marks["ej_mark"]

        # TODO: once this log line is in current shipping, remove oldcode if statement
        if not oldcode:
            ej_mark.wait_for_log_contains_from_mark("Completed initialization of Event Journaler", timeout)
        else:
            ej_mark.wait_for_log_contains_from_mark("Entering the main loop", timeout)

    def wait_for_deviceisolation_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        # TODO: once deviceisolation is in current shipping, remove this if statement
        if oldcode:
            if not os.path.isfile(self.deviceisolation_log):
                return
        deviceisolation_mark = log_marks["deviceisolation_mark"]

        deviceisolation_mark.wait_for_log_contains_from_mark("Completed initialization of Device Isolation", timeout)

    def wait_for_liveresponse_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        liveresponse_mark = log_marks["liveresponse_mark"]

        # TODO: once this log line is in current shipping, remove oldcode if statement
        if not oldcode:
            liveresponse_mark.wait_for_log_contains_from_mark("Completed initialization of Live Response", timeout)
        else:
            liveresponse_mark.wait_for_log_contains_from_mark("Entering the main loop", timeout)

    def wait_for_rtd_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 60):
        rtd_mark = log_marks["rtd_mark"]

        # Just try to find one or the other
        try:
            rtd_mark.wait_for_log_contains_from_mark("Analytics started processing telemetry", timeout)
        except:
            rtd_mark.wait_for_log_contains_from_mark("Sophos Runtime Detections Plugin version", timeout)

    def wait_for_av_logs_to_indicate_plugin_is_ready(self, log_marks: dict, timeout: int = 30, oldcode: bool = False):
        sophos_threat_detector_mark = log_marks["sophos_threat_detector_mark"]
        av_mark = log_marks["av_mark"]
        oa_mark = log_marks["oa_mark"]
        safestore_mark = log_marks["safestore_mark"]

        # TODO: once this log line is in current shipping, remove oldcode if statement
        if oldcode:
            sophos_threat_detector_mark.wait_for_log_contains_from_mark("Preparing to enter chroot at", timeout)
            av_mark.wait_for_log_contains_from_mark("Starting the main program loop", timeout)
            oa_mark.wait_for_log_contains_from_mark("Control Server Socket is at", timeout)
        else:
            sophos_threat_detector_mark.wait_for_log_contains_from_mark("Completed initialization of Sophos Threat Detector", timeout)
            av_mark.wait_for_log_contains_from_mark("Completed initialization of AV", timeout)
            oa_mark.wait_for_log_contains_from_mark("Completed initialization of Sophos On Access Process", timeout)

        safestore_mark.wait_for_log_contains_from_mark("SafeStore started", timeout)

    def wait_for_plugins_logs_to_indicate_plugins_are_ready(self, log_marks: dict, timeout: int = 60, oldcode : bool = False):
        self.wait_for_ej_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_deviceisolation_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_liveresponse_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_rtd_logs_to_indicate_plugin_is_ready(log_marks, timeout)
        self.wait_for_av_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_base_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_response_action_logs_to_indicate_plugin_is_ready(log_marks, timeout, oldcode)
        self.wait_for_edr_logs_to_indicate_plugin_is_ready(log_marks, timeout)

    def clear_log_marks(self, log_marks: dict):
        # This is for the specific case in a downgrade test where, after downgrading the plugin log file has the same
        # inode as before downgrading and the original log mark does not end up outside the new log file.
        # Resulting in no errors being thrown but the log mark can end up after the completed initialization log line
        # for the plugin.
        # position argument being set to 0 will result in __m_override_position = 0
        # When getting the contents of the file, __m_override_position will be used to seek through the file
        # since it will be 0, the entire file will be read
        for key, mark in log_marks.items():
            log_marks[key] = LogHandler.LogMark(mark.get_log_path(), 0)
