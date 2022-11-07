#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2021-2022 Sophos Ltd
# All rights reserved.
import os
import glob
import re
import six
import subprocess
import sys
import time
import typing

from robot.api import logger
import robot.libraries.BuiltIn

try:
    from . import LogHandler
except ImportError:
    import LogHandler


def _ensure_str(s):
    return six.ensure_str(s, "UTF-8", errors="backslashreplace")


def _get_log_contents(path_to_log):
    try:
        with open(path_to_log, "rb") as log:
            contents = log.read()
    except FileNotFoundError:
        return None
    except EnvironmentError:
        return None
    return contents.decode("UTF-8", errors='backslashreplace')


def _log_contains_in_order(log_location, log_name, args, log_finds=True):
    contents = _get_log_contents(log_location)
    index = 0
    for string in args:
        index = contents.find(string, index)
        if index != -1:
            if log_finds:
                logger.info("{} log contains {}".format(log_name, string))
            index = index + len(string)
        else:
            raise AssertionError("Remainder of {} log doesn't contain {}".format(log_name, string))


def _get_variable(var_name, default_value=None):
    try:
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % var_name) or default_value
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


def _get_sophos_install():
    return _get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")


def _mark_expected_errors_in_log(log_location, *error_messages):
    error_string = "ERROR"
    mark_string = "expected-error"

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
        contents = "".join(new_lines)

        if contents != original_contents:
            logger.info("Marking: " + logfile)
            with open(logfile, "w") as log:
                log.write(contents)


class LogUtils(object):
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'

    def __init__(self):
        self.tmp_path = os.path.join(".", "tmp")

        # Get Robot variables.
        self.install_path = _get_variable("SOPHOS_INSTALL", os.path.join("/", "opt", "sophos-spl"))
        self.router_path = _get_variable("MCS_DIR", os.path.join(self.install_path, "base", "mcs"))
        self.base_logs_dir = _get_variable("BASE_LOGS_DIR", os.path.join(self.install_path, "logs", "base"))
        self.av_plugin_logs_dir = os.path.join(self.install_path, "plugins", "av", "log")
        self.thin_install_log = os.path.join(self.tmp_path, "thin_installer", "ThinInstaller.log")
        self.suldownloader_log = os.path.join(self.base_logs_dir, "suldownloader.log")
        self.update_scheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "updatescheduler.log")
        self.register_log = os.path.join(self.base_logs_dir, "register_central.log")
        self.mdr_log = os.path.join(self.install_path, "plugins", "mtr", "log", "mtr.log")
        self.edr_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr.log")
        self.osquery_watcher_log = os.path.join(self.install_path, "plugins", "mtr", "dbos", "data", "logs", "osquery.watcher.log")
        self.sophos_threat_detector_log = os.path.join(self.install_path, "plugins", "av", "chroot", "log", "sophos_threat_detector.log")
        self.susi_debug_log = os.path.join(self.install_path, "plugins", "av", "chroot", "log", "susi_debug.log")
        self.av_log = os.path.join(self.av_plugin_logs_dir, "av.log")
        self.oa_log = os.path.join(self.av_plugin_logs_dir, "soapd.log")
        self.cloud_server_log = os.path.join(self.tmp_path, "cloudServer.log")
        self.marked_mcsrouter_logs = 0
        self.marked_mcs_envelope_logs = 0
        self.marked_watchdog_log = 0
        self.marked_managementagent_logs = 0
        self.marked_av_log = 0
        self.marked_sophos_threat_detector_log = 0
        self.__m_marked_log_position = {}

        self.__m_pending_mark_expected_errors = {}
        self.__m_log_handlers = {}

    def log_contains_in_order(self, log_location, log_name, args, log_finds=True):
        return _log_contains_in_order(log_location, log_name, args, log_finds)

    def get_register_central_log(self):
        return self.register_log

    def dump_logs(self):
        self.dump_log(self.oa_log)
        self.dump_log(self.av_log)
        self.dump_log(self.sophos_threat_detector_log)
        self.dump_watchdog_log()

    def dump_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "rb") as f:
                    logger.info("file: " + str(filename))
                    lines = f.readlines()
                    lines = [line.decode("UTF-8", errors="backslashreplace") for line in lines]
                    logger.info(u''.join(lines))
            except Exception as e:
                logger.info("Failed to read file: {}".format(e))
        else:
            logger.info("File %s does not exist" % str(filename))

    def dump_log_on_failure(self, filename):
        robot.libraries.BuiltIn.BuiltIn().run_keyword_if_test_failed("LogUtils.Dump Log", filename)

    def dump_cloud_server_log(self):
        server_log = self.cloud_server_log
        self.dump_log(server_log)

    def check_log_contains(self, string_to_contain, path_to_log, log_name=None):
        if log_name is None:
            log_name = os.path.basename(path_to_log)

        if not (os.path.isfile(path_to_log)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, path_to_log))

        contents = _get_log_contents(path_to_log)
        if string_to_contain not in contents:
            self.dump_log(path_to_log)
            raise AssertionError("{} Log at \"{}\" does not contain '{}'".format(log_name, path_to_log, string_to_contain))

    def file_log_contains_once(self, path, expected):
        """
        Check a log contains a string precisely once
        :param path:
        :param expected:
        :return:
        """
        log_name = os.path.basename(path)

        if not (os.path.isfile(path)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, path))

        contents = _get_log_contents(path)
        count = contents.count(expected)
        if count == 0:
            self.dump_log(path)
            raise AssertionError("{} Log at \"{}\" does not contain: {}".format(log_name, path, expected))

        if count > 1:
            self.dump_log(path)
            raise AssertionError("{} Log at \"{}\" contains {} repetitions: {}".format(log_name, path, count, expected))

    def file_log_contains(self, path, expected):
        """
        Reimplement:
File Log Contains
    [Arguments]  ${path}  ${input}
    File Should Exist  ${path}
    ${content} =  Get File   ${path}  encoding_errors=replace
    Should Contain  ${content}  ${input}

        Without error cases

        :param path:
        :param expected:
        :return:
        """
        return self.check_log_contains(expected, path)

    def over_next_15_seconds_ensure_log_does_not_contain(self, path_to_log, unexpected, period=15, log_name=None):
        log_name = log_name or os.path.basename(path_to_log)
        end = time.time() + period
        while time.time() < end:
            contents = _get_log_contents(path_to_log) or ""
            if unexpected in contents:
                raise AssertionError("{} Log at \"{}\" contains: {}".format(log_name, path_to_log, unexpected))
            time.sleep(1)
        logger.info("{} Log at \"{}\" does not contain: {}".format(log_name, path_to_log, unexpected))

    def check_log_and_return_nth_occurrence_between_strings(self, string_to_contain_start, string_to_contain_end, path_to_log,
                                                            occurs=1):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError("Log file {} does not exist".format(path_to_log))

        occurs_int = int(occurs)

        contents = _get_log_contents(path_to_log)

        index2 = 0
        while True:
            index1 = contents.find(string_to_contain_start, index2)
            if index1 == -1:
                break
            index2 = contents.find(string_to_contain_end, index1 + len(string_to_contain_start))
            logger.info("Index2 = {}".format(index2))
            if index2 == -1:
                break
            index2 = index2 + len(string_to_contain_end)
            occurs_int -= 1
            if occurs_int == 0:
                return contents[index1:index2]

        self.dump_log(path_to_log)
        raise AssertionError(
            "Log at \"{}\" does not contain following string pair {} - {} times : {}".format(path_to_log,
                                                                                             string_to_contain_start,
                                                                                             string_to_contain_end,
                                                                                             occurs))

    def check_mcs_envelope_for_status_same_in_at_least_one_of_the_following_statuses_sent_with_the_prior_being_noref(self, *status_numbers):
        # status_numbers should be integers
        # will check all statuses in order given to see if they are res=same
        # will pass when res=same is found
        # will fail on a status where res is neither same nor noref
        # the purpose of this function is to solve a race condition in a flakey test
        statuses = []
        for status_number in status_numbers:
            try:
                statuses.append(self.check_log_and_return_nth_occurrence_between_strings("<status><appId>ALC</appId>", "</status>", "{}/logs/base/sophosspl/mcs_envelope.log".format(self.install_path),  int(status_number)))
            except AssertionError:
                logger.info("Failed to get string for status {}".format(status_number))

        status_noref = "Res=&amp;quot;NoRef&amp;quot;"
        status_same = "Res=&amp;quot;Same&amp;quot;"
        count = 0
        for status in statuses:
            if status_same in status:
                logger.debug("got status: same")
                return int(status_numbers[count])
            elif status_noref not in status:
                logger.debug("got unexpected status")
                raise AssertionError("Status: {}\n was neither {} or {}".format(status, status_noref, status_same))
            count += 1
        raise AssertionError("Failed to find {} in statuses: {}".format(status_same, status_numbers))

    def check_log_does_not_contain(self, string_not_to_contain, path_to_log, log_name):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, path_to_log))

        contents = _get_log_contents(path_to_log)
        if string_not_to_contain in contents:
            self.dump_log(path_to_log)
            raise AssertionError("{} Log at \"{}\" does contain: {}".format(log_name, path_to_log, string_not_to_contain))

    def check_log_contains_more_than_one_pair_of_strings_in_order(self, string_one, string_two, path_to_log, log_name):
        logger.info("Strings are {} {}".format(string_one, string_two))
        if not (os.path.isfile(path_to_log)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, path_to_log))

        contents = _get_log_contents(path_to_log)

        first = True
        index2 = 0
        while True:
            index1 = contents.find(string_one, index2)
            logger.info("Index1 = {}".format(index1))
            if index1 == -1:
                if first:
                    raise AssertionError(
                        "{} Log at \"{}\" does contain first string in pair at all: {} - {}".format(log_name, path_to_log,
                                                                                                    string_one,
                                                                                                    string_two))
                else:
                    break
            index1 = index1 + len(string_one)
            index2 = contents.find(string_two, index1)
            logger.info("Index2 = {}".format(index2))
            if index2 == -1:
                self.dump_log(path_to_log)
                raise AssertionError(
                    "{} Log at \"{}\" does contain pair of strings in order: {} - {}".format(log_name, path_to_log,
                                                                                             string_one, string_two))
            index2 = index2 + len(string_two)
            first = False

    def check_log_contains_data_from_multisend(self, log_root, subscriber, publisher_num, data_items, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            data = []
            for datum in range(0, int(data_items)):
                data.append(
                    "Sub {} received message: ['{}', 'Data From publisher {} Num {}']".format(subscriber, channel_name,
                                                                                              publisher_id, datum))
            _log_contains_in_order(os.path.join(log_root, "fake_multi_subscriber.log"),
                                   "Fake Multi-Subscriber Log",
                                   data,
                                   False)

    def check_multiple_logs_contains_data_from_multisend(self, log_root, publisher_num, subscriber_num, data_items,
                                                         channel_name_log, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                data = []
                for datum in range(0, int(data_items)):
                    data.append("Sub Subscriber_{}_{} received message: ['{}', 'Data From publisher {} Num {}']".format(
                        subscriber_id, channel_name_log, channel_name, publisher_id, datum))
                _log_contains_in_order(
                    os.path.join(log_root, "Subscriber_{}_{}.log".format(subscriber_id, channel_name_log)),
                    "Fake Subscriber {} {} Log".format(subscriber_id, channel_name_log),
                    data,
                    False)

    def check_multiple_logs_do_not_contain_data_from_multisend(self, log_root, publisher_num, subscriber_num,
                                                               channel_name_listened_to, channel_name_not_listened_to):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                self.check_log_does_not_contain(
                    "Sub Subscriber_{}_{} received message: ['{}', 'Data From publisher {} Num".format(subscriber_id,
                                                                                                       channel_name_listened_to,
                                                                                                       channel_name_not_listened_to,
                                                                                                       publisher_id),
                    os.path.join(log_root, "Subscriber_{}_{}.log".format(subscriber_id, channel_name_listened_to)),
                    "Fake Subscriber {} {} Log".format(subscriber_id, channel_name_listened_to))

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

    def check_all_product_logs_do_not_contain_string(self, string_to_find, search_list):
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combined_files = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        flat_files = [item for sublist in combined_files for item in sublist]
        list_of_logs_containing_string = []
        for filepath in flat_files:
            if any(elem in filepath for elem in ("mcs", "suldownloader", "updatescheduler")):
                logger.info("Skipping file: {}".format(filepath))
                continue
            num_occurrence = self.get_number_of_occurrences_of_substring_in_log(filepath, string_to_find)
            if num_occurrence > 0:
                self.log_string_if_found(string_to_find, filepath)
                list_of_logs_containing_string.append("{} - {} times".format(filepath, num_occurrence))
                robot.libraries.BuiltIn.BuiltIn().run_keyword("LogUtils.Dump Log", filepath)
                # Edit file to avoid cascading failures
                replacement = "KNOWN" + "!" * len(string_to_find)
                replacement = replacement[:len(string_to_find)]  # same length as string_to_find
                self.replace_all_in_file(filepath, string_to_find, replacement)

        if list_of_logs_containing_string:
            raise AssertionError("These program logs contain {}:\n {}".format(string_to_find, list_of_logs_containing_string))

    def check_all_product_logs_do_not_contain_error(self):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*", "plugins/av/log/sophos_threat_detector/sophos_threat_detector*.log*"]
        logger.info("Re-apply expected errors")
        for log_location, error_messages in self.__m_pending_mark_expected_errors.items():
            _mark_expected_errors_in_log(log_location, *error_messages)

        self.check_all_product_logs_do_not_contain_string("]   ERROR [", search_list)
        self.__m_pending_mark_expected_errors = {}

    def check_all_product_logs_do_not_contain_critical(self):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*", "plugins/av/log/sophos_threat_detector/*.log*"]
        self.check_all_product_logs_do_not_contain_string("CRITICAL", search_list)

    def get_number_of_occurrences_of_substring_in_log(self, log_location, substring) -> int:
        contents = _get_log_contents(log_location)
        return self.get_number_of_occurrences_of_substring_in_string(contents, substring)

    def get_number_of_occurrences_of_substring_in_string(self, string, substring, use_regex=False) -> int:
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

    # require that special characters are escaped with '\' [ /, +, *, ., (, ) etc ]
    def get_number_of_occurrences_of_regex_in_string(self, string, reg_expresion_str) -> int:
        import re
        reg_expression = re.compile(reg_expresion_str)
        log_occurrences = reg_expression.findall(string)
        return len(log_occurrences)

    def check_string_matching_regex_in_file(self, file_path, reg_expression_str):
        if not os.path.exists(file_path):
            raise AssertionError("File not found '{}'".format(file_path))
        if self.get_number_of_occurrences_of_regex_in_string(_get_log_contents(file_path), reg_expression_str) < 1:
            self.dump_log(file_path)
            raise AssertionError(
                "The file: '{}', did not have any lines match the regex: '{}'".format(file_path, reg_expression_str))

    def mark_expected_error_in_log(self, log_location, error_message):
        self.__m_pending_mark_expected_errors.setdefault(log_location, []).append(error_message)
        _mark_expected_errors_in_log(log_location, error_message)

    def dump_thininstaller_log(self):
        self.dump_log(self.thin_install_log)

    def dump_suldownloader_log(self):
        self.dump_log(self.suldownloader_log)

    def check_thininstaller_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.thin_install_log, "Thin Installer")

    def check_suldownloader_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.suldownloader_log, "Suldownloader")

    def osquery_watcher_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.osquery_watcher_log, "osquery watcher log")

    def osquery_watcher_log_should_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.osquery_watcher_log, "osquery watcher log")

    def check_thininstaller_log_does_not_contain(self, string_not_to_contain):
        self.check_log_does_not_contain(string_not_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_contains_in_order(self, *args):
        _log_contains_in_order(self.thin_install_log, "Thin installer", args)

    def cloud_server_log_should_contain_multi_line(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.cloud_server_log, "cloud server log")

    def cloud_server_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.cloud_server_log, "cloud server log")

    def remove_thininstaller_log(self):
        if os.path.isfile(self.thin_install_log):
            os.remove(self.thin_install_log)

    def remove_mtr_plugin_log(self):
        if os.path.isfile(self.mdr_log):
            os.remove(self.mdr_log)

    def check_suldownloader_log_contains_in_order(self, *args):
        _log_contains_in_order(self.suldownloader_log, "Suldownloader", args)

    def check_mcsrouter_log_contains_in_order(self, *args):
        _log_contains_in_order(self.mcs_router_log(), "MCS Router", args)

    def check_log_contains_in_order(self, log_path, *args):
        _log_contains_in_order(log_path, log_path, args)

    def check_mcsrouter_does_not_contain_critical_exceptions(self):
        if os.path.exists(self.mcs_router_log()):
            self.check_log_does_not_contain("Caught exception at top-level;", self.mcs_router_log(), "MCS router")

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

    def dump_register_central_log(self):
        self.dump_log(self.register_log)

    def check_register_central_log_contains_in_order(self, *args):
        _log_contains_in_order(self.register_log, "Register Central", args)

    def check_mdr_log_contains_in_order(self, *args):
        _log_contains_in_order(self.mdr_log, "MDR", args)

    def check_register_central_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.register_log, "Register Central")

    def check_edr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.edr_log, "EDR")

    def check_mdr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.mdr_log, "MDR")

    def mcs_router_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")

    def mcs_envelope_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")

    def watchdog_log(self):
        return os.path.join(self.base_logs_dir, "watchdog.log")

    def managementagent_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "sophos_managementagent.log")

    def scheduled_scan_log(self, scanname="MyScan"):
        return os.path.join(self.av_plugin_logs_dir, scanname+".log")

    def soapd_log(self):
        return os.path.join(self.av_plugin_logs_dir, "soapd.log")

    def dump_mcsrouter_log(self):
        mcsrouter_log = self.mcs_router_log()
        self.dump_log(mcsrouter_log)

    def dump_mcs_envelope_log(self):
        mcs_envelope_log = self.mcs_envelope_log()
        self.dump_log(mcs_envelope_log)

    def dump_watchdog_log(self):
        watchdog_log = self.watchdog_log()
        self.dump_log(watchdog_log)

    def dump_managementagent_log(self):
        managementagent_log = self.managementagent_log()
        self.dump_log(managementagent_log)

    def dump_scheduled_scan_log(self, scanname="MyScan"):
        scan_log = self.scheduled_scan_log(scanname)
        self.dump_log(scan_log)

    def count_optional_file_log_lines(self, filename):
        contents = _get_log_contents(filename)
        if contents is None:
            return 0
        return len(contents.splitlines())

    def mark_mcs_envelope_log(self):
        mcs_envelope_log = self.mcs_envelope_log()
        contents = _get_log_contents(mcs_envelope_log)
        self.marked_mcs_envelope_logs = len(contents)

    def mark_mcsrouter_log(self):
        mcsrouter_log = self.mcs_router_log()
        contents = _get_log_contents(mcsrouter_log)
        self.marked_mcsrouter_logs = len(contents)

    def mark_watchdog_log(self):
        watchdog_log = self.watchdog_log()
        contents = _get_log_contents(watchdog_log)
        self.marked_watchdog_log = len(contents)

    def get_marked_watchdog_log(self):
        watchdog_log = self.watchdog_log()
        contents = _get_log_contents(watchdog_log)
        return contents[self.marked_watchdog_log:]

    def dump_marked_watchdog_log(self):
        contents = self.get_marked_watchdog_log()
        logger.info("Marked Watchdog Log:")
        logger.info(contents)

    def mark_managementagent_log(self):
        managementagent_log = self.managementagent_log()
        contents = _get_log_contents(managementagent_log)
        self.marked_managementagent_logs = len(contents)

    def mark_av_log(self):
        contents = _get_log_contents(self.av_log)
        if contents is None:
            self.marked_av_log = 0
            return 0

        self.marked_av_log = len(contents)  # bytes
        return len(contents.splitlines())

    def mark_sophos_threat_detector_log(self):
        sophos_threat_detector_log = self.sophos_threat_detector_log
        contents = _get_log_contents(sophos_threat_detector_log)
        self.marked_sophos_threat_detector_log = len(contents)

    def get_marked_sophos_threat_detector_log(self, mark=None):
        if mark is None:
            mark = self.marked_sophos_threat_detector_log
        sophos_threat_detector_log = self.sophos_threat_detector_log
        contents = _get_log_contents(sophos_threat_detector_log)
        return contents[mark:]

    def dump_marked_sophos_threat_detector_log(self):
        contents = self.get_marked_sophos_threat_detector_log()
        logger.info("Marked Sophos Threat Detector Log:")
        logger.info(contents)

    def get_marked_av_log(self):
        av_log = self.av_log
        contents = _get_log_contents(av_log)
        contents = contents[self.marked_av_log:]
        return contents

    def dump_marked_av_log(self):
        contents = self.get_marked_av_log()
        logger.info("Marked AV Log:")
        logger.info(contents)

    def check_marked_av_log_contains(self, string_to_contain, mark):
        """
        :param string_to_contain: string to look for
        :param mark: IGNORED - number of lines to skip
        :return: True if string_to_contain in marked log
        """
        contents = self.get_marked_av_log()

        if string_to_contain not in contents:
            self.dump_marked_av_log()
            raise AssertionError("av.log log did not contain: '" +
                                 string_to_contain+"', starting from '"+contents[:50]+"'")

    def check_marked_sophos_threat_detector_log_contains(self, string_to_contain, mark=None):
        if isinstance(mark, LogHandler.LogMark):
            contents = self.get_sophos_threat_detector_log_after_mark(mark)
        else:
            contents = self.get_marked_sophos_threat_detector_log(mark)

        contents = _ensure_str(contents)

        if string_to_contain not in contents:
            self.dump_marked_sophos_threat_detector_log()
            raise AssertionError("sophos_threat_detector.log log did not contain: " + string_to_contain)
        return contents

    def verify_sophos_threat_detector_log_line_is_level(self, expected_level, string_to_check, mark):
        contents = self.check_marked_sophos_threat_detector_log_contains(string_to_check, mark)
        # 9296    [2022-07-29T08:45:34.335]    WARN [9369807616] ThreatScanner <> Failed to scan /home/vagrant/this/is/a/directory/for/scanning/password_protected.7z/eicar.com as it is password protected
        line_re = re.compile(r"^\d+\s+\[\S+]\s+(\w+)\s+.*?"+re.escape(string_to_check)+r".*?$", flags=re.MULTILINE)
        found = False
        for mo in line_re.finditer(contents):
            found = True
            logger.debug("Found line: %s" % mo.group(0))
            level = mo.group(1)
            logger.debug("Level: %s" % level)
            if level != expected_level:
                raise AssertionError("sophos_threat_detector log contains expected string \"{}\" at unexpected level {} (expected {})".format(
                    string_to_check,
                    level,
                    expected_level
                ))
        if not found:
            logger.error("Regex failed to match")

    def verify_sophos_threat_detector_log_line_is_informational(self, string_to_check):
        contents = self.check_marked_sophos_threat_detector_log_contains(string_to_check)

        # 13819   [2021-05-20T10:37:39.084]    INFO [6594151360] SophosThreatDetectorImpl <> Sophos Threat Detector received SIGTERM - shutting down
        line_re = re.compile(r"^\d+\s+\S+\s+(\w+).*?"+re.escape(string_to_check)+r".*?$", flags=re.MULTILINE)
        found = False
        for mo in line_re.finditer(contents):
            found = True
            logger.info("Found line: %s" % mo.group(0))
            level = mo.group(1)
            logger.info("Level: %s" % level)
            if level not in ("DEBUG", "INFO", "SUPPORT", "SPRT"):
                raise AssertionError("sophos_threat_detector.log bad level %s in line %s" % (level, mo.group(0)))
        if not found:
            raise AssertionError("Regex failed to find %s" % string_to_check)

    def check_marked_mcs_envelope_log_contains(self, string_to_contain):
        mcs_envelope_log = self.mcs_envelope_log()
        contents = _get_log_contents(mcs_envelope_log)
        contents = contents[self.marked_mcs_envelope_logs:]

        if string_to_contain not in contents:
            self.dump_mcs_envelope_log()
            raise AssertionError("MCS Envelope log did not contain: " + string_to_contain)

    def check_marked_mcsrouter_log_contains(self, string_to_contain):
        mcsrouter_log = self.mcs_router_log()
        contents = _get_log_contents(mcsrouter_log)
        contents = contents[self.marked_mcsrouter_logs:]

        if string_to_contain not in contents:
            self.dump_mcsrouter_log()
            raise AssertionError("MCS Router log did not contain: " + string_to_contain)

    def check_marked_watchdog_log_contains(self, string_to_contain):
        watchdog_log = self.watchdog_log()
        contents = _get_log_contents(watchdog_log)

        contents = contents[self.marked_watchdog_log:]

        if string_to_contain not in contents:
            self.dump_watchdog_log()
            raise AssertionError("Marked watchdog log did not contain: " + string_to_contain)

    def check_marked_managementagent_log_contains(self, string_to_contain):
        managementagent_log = self.managementagent_log()
        contents = _get_log_contents(managementagent_log)

        contents = contents[self.marked_managementagent_logs:]

        if string_to_contain not in contents:
            self.dump_managementagent_log()
            raise AssertionError("Marked managementagent log did not contain: " + string_to_contain)

    def check_marked_managementagent_log_contains_regex(self, string_to_contain):
        managementagent_log = self.managementagent_log()
        contents = _get_log_contents(managementagent_log)

        contents = contents[self.marked_managementagent_logs:]

        import re
        if not re.search(string_to_contain, contents):
            self.dump_managementagent_log()
            raise AssertionError("Marked managementagent log did not contain: " + string_to_contain)

    def check_marked_mcsrouter_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        mcsrouter_log = os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")
        contents = _get_log_contents(mcsrouter_log)

        contents = contents[self.marked_mcsrouter_logs:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                "McsRouter Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain,
                                                                                              num_occurrences,
                                                                                              expected_occurrence))

    def check_log_contains_string_n_times(self, log_path, log_name, string_to_contain, expected_occurrence):
        contents = _get_log_contents(log_path)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                "{} Contains: \"{}\" - {} times not the requested {} times".format(log_name,
                                                                                   string_to_contain,
                                                                                   num_occurrences,
                                                                                   expected_occurrence))

    def check_mcs_envelope_log_contains_regex_string_n_times(self, string_to_contain, expected_occurrence):
        self.check_mcs_envelope_log_contains_string_n_times(string_to_contain, expected_occurrence, True)

    def check_mcs_envelope_log_contains_string_n_times(self, string_to_contain, expected_occurrence, use_regex=False):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        contents = _get_log_contents(mcs_envelope_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError("mcs_envelope Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain, num_occurrences, expected_occurrence))

    def check_updatescheduler_log_contains(self, string_to_contain):
        updatescheduler_log = self.update_scheduler_log
        self.check_log_contains(string_to_contain, updatescheduler_log, "UpdateScheduler")
        logger.info(updatescheduler_log)

    def check_mcsrouter_log_contains(self, string_to_contain):
        mcsrouter_log = self.mcs_router_log()
        self.check_log_contains(string_to_contain, mcsrouter_log, "MCS Router")
        logger.info(mcsrouter_log)

    def check_mcsrouter_log_does_not_contain(self, string_to_not_contain):
        mcsrouter_log = self.mcs_router_log()
        self.check_log_does_not_contain(string_to_not_contain, mcsrouter_log, "MCS Router")

    def check_watchdog_log_contains(self, string_to_contain):
        watchdog_log = self.watchdog_log()
        self.check_log_contains(string_to_contain, watchdog_log, "Watchdog")

    def check_watchdog_log_does_not_contain(self, string_to_not_contain):
        watchdog_log = self.watchdog_log()
        self.check_log_does_not_contain(string_to_not_contain, watchdog_log, "Watchdog")

    def check_mtr_log_does_not_contain(self, string_to_not_contain):
        mdr_log = self.mdr_log
        self.check_log_does_not_contain(string_to_not_contain, mdr_log, "MTR Plugin Log")

    def verify_message_relay_failure_in_order(self, *messagerelays, **kwargs):
        mcsrouter_log = self.mcs_router_log()
        MCS_ADDRESS = kwargs.get("MCS_ADDRESS", "mcs.sandbox.sophos:443")
        targets = []
        for mr in messagerelays:
            targets.append("Failed connection with message relay via {} to {}:".format(mr, MCS_ADDRESS))

        _log_contains_in_order(mcsrouter_log, "mcs_router", targets)

    def check_mcsenvelope_log_contains(self, string_to_contain):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.check_log_contains(string_to_contain, mcs_envelope_log, "MCS Envelope")

    def check_mcsenvelope_log_does_not_contain(self, string_to_not_contain):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.check_log_does_not_contain(string_to_not_contain, mcs_envelope_log, "MCS Envelope")

    def check_management_agent_log_contains(self, string_to_contain):
        management_agent_log = os.path.join(self.base_logs_dir, "sophosspl", "sophos_managementagent.log")
        self.check_log_contains(string_to_contain, management_agent_log, "Management Agent")

    def does_management_agent_log_contain(self, string_to_contain):
        try:
            self.check_management_agent_log_contains(string_to_contain)
            return True
        except AssertionError:
            return False

    def check_mcsenvelope_log_contains_in_order(self, *args):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        _log_contains_in_order(mcs_envelope_log, "MCS Envelope", args)

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
        if not os.path.isdir(mcs_events_dir):
            return
        for filename in os.listdir(mcs_events_dir):
            fullpath = os.path.join(mcs_events_dir, filename)
            self.dump_log(fullpath)

    def dump_all_processes(self):
        pstree = '/usr/bin/pstree'
        if os.path.isfile(pstree):
            logger.info("%s", subprocess.check_output([pstree, '-ap'], stderr=subprocess.PIPE))
        else:
            logger.info("%s", subprocess.check_output(['/bin/ps', '-elf'], stderr=subprocess.PIPE))
        try:
            top_path = [candidate for candidate in ['/bin/top', '/usr/bin/top'] if os.path.isfile(candidate)][0]
            logger.info("%s", subprocess.check_output([top_path, '-bHn1', '-o', '+%CPU', '-o', '+%MEM']))
        except Exception as ex:
            logger.warn("Failed to run top: %s", str(ex))

    # Mainly for debugging; this function will add a marker line to all log files so that we can easily view logs
    # from each part of the tests, a custom tag can also be included.
    def mark_all_logs(self, tag=""):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*"]
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combined_files = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        log_files = [item for sublist in combined_files for item in sublist]
        for filepath in log_files:
            with open(filepath, 'a') as log_file:
                log_file.write('-{}----------------------------------------------------------\n'.format(tag))

    def get_version_number_from_ini_file(self, file):
        with open(file) as ini_file:
            lines = ini_file.readlines()
            version_text = "PRODUCT_VERSION = "
            for line in lines:
                if version_text in line:
                    version_string = line[len(version_text):]
                    logger.info("Found version string: {}".format(version_string))
                    return version_string.strip()
        logger.error("Version String Not Found")
        raise AssertionError("Version String Not Found")

    @staticmethod
    def all_should_be_equal(*args):
        assert len(args) > 1, "Error: should have more than 1 argument"
        master = args[0]
        for arg in args[1:]:
            if arg != master:
                raise AssertionError("Not all items are equal in: {}".format(args))

    @staticmethod
    def get_logger_conf_path():
        install_dir = _get_sophos_install()
        logger_conf_pattern = os.path.join(install_dir, 'base/etc/logger.conf.')
        files = glob.glob(logger_conf_pattern + '*')
        if files:
            return files[0]
        else:
            return logger_conf_pattern + '0'

    def get_latest_mount_inclusion_count_from_on_access_log(self, markLines):
        contents = _get_log_contents(self.soapd_log())
        contents = contents.splitlines(False)
        if len(contents) > markLines:
            contents = contents[markLines:]

        LOG_RE = re.compile(r".*Including (\d+) mount points in on-access scanning.*")
        contents.reverse()  # Search for the latest
        for line in contents:
            mo = LOG_RE.match(line)
            if mo:
                return int(mo.group(1))

        return None

    def get_log_handler(self, logpath) -> LogHandler.LogHandler:
        handler = self.__m_log_handlers.get(logpath, None)
        if handler is None:
            handler = LogHandler.LogHandler(logpath)
            self.__m_log_handlers[logpath] = handler
        return handler

    def mark_log_size(self, logpath) -> LogHandler.LogMark:
        handler = self.get_log_handler(logpath)
        mark = handler.get_mark()
        self.__m_marked_log_position[logpath] = mark  # Save the most recent marked position
        return mark

    def wait_for_log_contains_from_mark(self, mark: LogHandler.LogMark, expected, timeout=10) -> None:
        assert mark is not None
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_from_mark"
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def wait_for_log_contains_after_mark(self,
                                         logpath: typing.Union[str, bytes],
                                         expected: typing.Union[str, bytes],
                                         mark: LogHandler.LogMark,
                                         timeout=10) -> None:
        if mark is None:
            logger.error("No mark passed for wait_for_log_contains_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected, logpath))
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_log_contains_after_mark"
        mark.assert_is_good(logpath)
        return mark.wait_for_log_contains_from_mark(expected, timeout)

    def check_log_contains_after_mark(self, log_path, expected, mark):
        if mark is None:
            logger.error("No mark passed for check_log_contains_after_mark")
            raise AssertionError("No mark set to find %s in %s" % (expected, log_path))

        if isinstance(expected, str):
            expected = expected.encode("UTF-8")

        handler = self.get_log_handler(log_path)
        contents = handler.get_contents(mark)
        if expected in contents:
            return

        logger.error("Failed to find %s in %s" % (expected, log_path))
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

    def check_log_does_not_contain_after_mark(self, log_path, not_expected, mark):
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in check_log_does_not_contain_after_mark"
        not_expected = six.ensure_binary(not_expected, "UTF-8")
        handler = self.get_log_handler(log_path)
        contents = handler.get_contents(mark)
        if not_expected in contents:
            self.dump_marked_log(log_path, mark)
            raise AssertionError("Found %s in %s" % (not_expected, log_path))

########################################################################################################################
# On-Access Soapd Log
    def get_on_access_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.oa_log)

    def save_on_access_log_mark_at_start_of_test(self):
        robot.libraries.BuiltIn.BuiltIn().set_test_variable("${ON_ACCESS_LOG_MARK_FROM_START_OF_TEST}",
                                                            self.get_on_access_log_mark())

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

#####################################################################
# AV Log

    def get_av_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.av_log)

    def get_av_log_after_mark(self, mark):
        return self.get_log_after_mark(self.av_log, mark)

    def get_av_log_after_mark_as_unicode(self, mark):
        return _ensure_str(self.get_av_log_after_mark(mark))

    def wait_for_av_log_contains_after_mark(self, expected: str, mark: LogHandler.LogMark, timeout: int = 10):
        assert isinstance(mark, LogHandler.LogMark), "mark is not an instance of LogMark in wait_for_av_log_contains_after_mark"
        return self.wait_for_log_contains_after_mark(self.av_log, expected, mark, timeout=timeout)

    def check_av_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.av_log, not_expected, mark)

#####################################################################
# Sophos Threat Detector Log

    def get_sophos_threat_detector_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.sophos_threat_detector_log)

    def get_sophos_threat_detector_log_after_mark(self, mark):
        return self.get_log_after_mark(self.sophos_threat_detector_log, mark)

    def check_sophos_threat_detector_log_contains_after_mark(self, expected, mark):
        return self.check_log_contains_after_mark(self.sophos_threat_detector_log, expected, mark)

    def check_sophos_threat_detector_log_does_not_contain_after_mark(self, not_expected, mark):
        return self.check_log_does_not_contain_after_mark(self.sophos_threat_detector_log, not_expected, mark)

    def dump_sophos_threat_detector_log_after_mark(self, mark):
        return self.dump_marked_log(self.sophos_threat_detector_log, mark)

#####################################################################
# SUSI Debug Log

    def get_susi_debug_log_mark(self) -> LogHandler.LogMark:
        return self.mark_log_size(self.susi_debug_log)

    def get_susi_debug_log_after_mark(self, mark):
        return self.get_log_after_mark(self.susi_debug_log, mark)


def __main(argv):
    # write your tests here
    return 0


if __name__ == "__main__":
    sys.exit(__main(sys.argv))
