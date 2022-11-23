import configparser
import glob
import os
import shutil
import subprocess

import dateutil.parser
import robot.libraries.BuiltIn
from robot.api import logger


def get_log_contents(path_to_log):
    with open(path_to_log, "r") as log:
        contents = log.read()
    return contents


def log_contains_in_order(log_location, log_name, args, log_finds=True):
    contents = get_log_contents(log_location)
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
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % var_name)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(var_name, default_value)


class LogUtils(object):
    def __init__(self):
        self.tmp_path = os.path.join(".", "tmp")

        # Get Robot variables.
        self.install_path = get_variable("SOPHOS_INSTALL", os.path.join("/", "opt", "sophos-spl"))
        self.router_path = get_variable("MCS_DIR", os.path.join(self.install_path, "base", "mcs"))
        self.base_logs_dir = get_variable("BASE_LOGS_DIR", os.path.join(self.install_path, "logs", "base"))
        self.register_log = os.path.join(self.base_logs_dir, "register_central.log")
        self.suldownloader_log = os.path.join(self.base_logs_dir, "suldownloader.log")
        self.watchdog_log = os.path.join(self.base_logs_dir, "watchdog.log")
        self.comms_component_network_log = os.path.join(self.base_logs_dir, "sophos-spl-comms", "comms_network.log")
        self.comms_component_log = os.path.join(self.base_logs_dir, "sophosspl", "comms_component.log")
        self.managementagent_log = os.path.join(self.base_logs_dir, "sophosspl", "sophos_managementagent.log")
        self.mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        self.mcs_router_log = os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")
        self.tscheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "tscheduler.log")
        self.update_scheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "updatescheduler.log")
        self.av_log = os.path.join(self.install_path, "plugins", "av", "log", "av.log")
        self.sophos_threat_detector_log = os.path.join(self.install_path, "plugins", "av", "chroot", "log", "sophos_threat_detector.log")
        self.edr_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr.log")
        self.edr_osquery_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr_osquery.log")
        self.livequery_log = os.path.join(self.install_path, "plugins", "edr", "log", "livequery.log")
        self.ej_log = os.path.join(self.install_path, "plugins", "eventjournaler", "log", "eventjournaler.log")
        self.liveresponse_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "liveresponse.log")
        self.sessions_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "sessions.log")
        self.mdr_log = os.path.join(self.install_path, "plugins", "mtr", "log", "mtr.log")
        self.osquery_watcher_log = os.path.join(self.install_path, "plugins", "mtr", "dbos", "data", "logs", "osquery.watcher.log")
        self.cloud_server_log = os.path.join(self.tmp_path, "cloudServer.log")
        self.thin_install_log = os.path.join(self.tmp_path, "thin_installer", "ThinInstaller.log")
        self.marked_av_log = 0
        self.marked_edr_log = 0
        self.marked_edr_osquery_log = 0
        self.marked_livequery_log = 0
        self.marked_managementagent_log = 0
        self.marked_mcs_envelope_logs = 0
        self.marked_mcsrouter_logs = 0
        self.marked_sophos_threat_detector_log = 0
        self.marked_sul_logs = 0
        self.marked_update_scheduler_logs = 0
        self.marked_watchdog_logs = 0

    # Common Log Utils
    def get_log_line(self, string_to_contain, path_to_log):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {path_to_log} does not exist")

        with open(path_to_log, "r") as log:
            contents = log.readlines()

        for line in contents:
            if string_to_contain in line:
                return line

        raise AssertionError(f"Log at \"{path_to_log}\" does not contain: {string_to_contain}")

    def get_timestamp_of_log_line(self, string_to_contain, path_to_log):
        line = self.get_log_line(string_to_contain, path_to_log)
        timestamp = line.split("[")[1].split("]")[0]

        return dateutil.parser.isoparse(timestamp)

    def get_time_difference_between_two_log_lines(self, string_to_contain1, string_to_contain2, path_to_log):
        timestamp1 = self.get_timestamp_of_log_line(string_to_contain1, path_to_log)
        timestamp2 = self.get_timestamp_of_log_line(string_to_contain2, path_to_log)

        difference = timestamp2 - timestamp1

        return difference.total_seconds()

    def get_number_of_occurrences_of_substring_in_log(self, log_location, substring):
        contents = get_log_contents(log_location)
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
        if self.get_number_of_occurrences_of_regex_in_string(get_log_contents(file_path), reg_expression_str) < 1:
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

    def replace_string_in_file(self, old_string, new_string, file):
        if not os.path.isfile(file):
            raise AssertionError(f"File not found {file}")
        with open(file, "rt") as f:
            content = f.read()
        content = content.replace(old_string, new_string)
        with open(file, "wt") as f:
            f.write(content)

    def dump_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "r") as f:
                    logger.info(f"file: {str(filename)}")
                    logger.info(''.join(f.readlines()))
            except Exception as e:
                logger.info(f"Failed to read file: {e}")
        else:
            logger.info("File does not exist")

    def check_log_contains(self, string_to_contain, path_to_log, log_name):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {log_name} at location {path_to_log} does not exist ")

        contents = get_log_contents(path_to_log)
        if string_to_contain not in contents:
            self.dump_log(path_to_log)
            raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does not contain: {string_to_contain}")

    def check_log_does_not_contain(self, string_not_to_contain, path_to_log, log_name):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError(f"Log file {log_name} at location {path_to_log} does not exist ")

        contents = get_log_contents(path_to_log)
        if string_not_to_contain in contents:
            self.dump_log(path_to_log)
            raise AssertionError(f"{log_name} Log at \"{path_to_log}\" does contain: {string_not_to_contain}")

    def check_log_contains_string_n_times(self, log_path, log_name, string_to_contain, expected_occurrence):
        contents = get_log_contents(log_path)

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
        contents = get_log_contents(log_path)

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

        contents = get_log_contents(path_to_log)

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

        contents = get_log_contents(path_to_log)

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
        mark_string = "expected-error"
        tmp_log = "/tmp/log.log"
        shutil.copy(log_location, tmp_log)
        with open(log_location, "w") as log:
            log.write("")
        with open(tmp_log, "r") as log:
            while True:
                # Get next line from file
                line = log.readline()
                if not line:
                    break
                if error_message in line:
                    line = line.replace(error_string, mark_string)
                with open(log_location, "a") as new_log:
                    new_log.write(line)
        os.remove(tmp_log)

    def mark_expected_error_in_log(self, log_location, error_message):
        error_string = "ERROR"
        mark_string = "expected-error"
        tmp_log = "/tmp/log.log"
        shutil.copy(log_location, tmp_log)
        with open(log_location, "w") as log:
            log.write("")
        with open(tmp_log, "r") as log:
            while True:
                # Get next line from file
                line = log.readline()
                if not line:
                    break
                if error_message in line:
                    line = line.replace(error_string, mark_string)
                with open(log_location, "a") as newlog:
                    newlog.write(line)
        os.remove(tmp_log)

    def check_all_product_logs_do_not_contain_string(self, string_to_find):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*"]
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combined_files = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        flat_files = [item for sublist in combined_files for item in sublist]

        list_of_logs_containing_string = []
        for filepath in flat_files:
            num_occurrence = self.get_number_of_occurrences_of_substring_in_log(filepath, string_to_find)
            if num_occurrence > 0:
                list_of_logs_containing_string.append(f"{filepath} - {num_occurrence} times")
        if list_of_logs_containing_string:
            raise AssertionError(f"These program logs contain {string_to_find}:\n {list_of_logs_containing_string}")

    def check_all_product_logs_do_not_contain_critical(self):
        self.check_all_product_logs_do_not_contain_string("CRITICAL")

    def check_all_product_logs_do_not_contain_error(self):
        self.check_all_product_logs_do_not_contain_string("ERROR")

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

    def cloud_server_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.cloud_server_log, "cloud server log")

    def cloud_server_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.cloud_server_log, "cloud server log")

    # ThinInstaller Log Utils
    def dump_thininstaller_log(self):
        self.dump_log(self.thin_install_log)

    def remove_thininstaller_log(self):
        if os.path.isfile(self.thin_install_log):
            os.remove(self.thin_install_log)

    def check_thininstaller_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_does_not_contain(self, string_not_to_contain):
        self.check_log_does_not_contain(string_not_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_contains_in_order(self, *args):
        log_contains_in_order(self.thin_install_log, "Thin installer", args)

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
    def check_suldownloader_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.suldownloader_log, "Suldownloader")

    def check_suldownloader_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.suldownloader_log, "Suldownloader")

    def check_suldownloader_log_contains_in_order(self, *args):
        log_contains_in_order(self.suldownloader_log, "Suldownloader", args)

    def check_suldownloader_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = get_log_contents(self.suldownloader_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain,
                                                                               use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"suldownloader Log Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def mark_sul_log(self):
        contents = get_log_contents(self.suldownloader_log)
        self.marked_sul_logs = len(contents)

    def check_marked_sul_log_contains(self, string_to_contain):
        sul_log = self.suldownloader_log
        contents = get_log_contents(sul_log)

        contents = contents[self.marked_sul_logs:]

        if string_to_contain not in contents:
            self.dump_log(sul_log)
            raise AssertionError(f"SUL downloader log did not contain: {string_to_contain}")

    def check_marked_sul_log_does_not_contain(self, string_to_not_contain):
        sul_log = self.suldownloader_log
        contents = get_log_contents(sul_log)

        contents = contents[self.marked_sul_logs:]

        if string_to_not_contain in contents:
            self.dump_log(sul_log)
            raise AssertionError(f"SUL downloader log contains: {string_to_not_contain}")

    # Watchdog Log Utils
    def dump_watchdog_log(self):
        self.dump_log(self.watchdog_log)

    def mark_watchdog_log(self):
        contents = get_log_contents(self.watchdog_log)
        self.marked_watchdog_logs = len(contents)

    def check_watchdog_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.watchdog_log, "Watchdog")
        logger.info(self.watchdog_log)

    def check_watchdog_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.watchdog_log, "Watchdog")

    def check_marked_watchdog_log_contains(self, string_to_contain):
        contents = get_log_contents(self.watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain not in contents:
            self.dump_watchdog_log()
            raise AssertionError(f"Marked watchdog log did not contain: {string_to_contain}")

    def check_marked_watchdog_log_does_not_contain(self, string_to_contain):
        contents = get_log_contents(self.watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain in contents:
            self.dump_watchdog_log()
            raise AssertionError(f"Marked watchdog log did not contain: {string_to_contain}")

    # Comms Component Log Utils
    def check_comms_component_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.comms_component_log, "Comms Component Log")
        logger.info(self.comms_component_log)

    def check_comms_component_log_does_not_contain_error(self):
        self.check_log_does_not_contain("ERROR", self.comms_component_log, "Comms Component")
        logger.info(self.comms_component_log)

    def check_comms_component_network_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.comms_component_network_log, "Comms Component Network Log")
        logger.info(self.comms_component_network_log)

    def check_comms_component_network_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.comms_component_network_log, "Comms Component Network Log")

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
        contents = get_log_contents(mcs_envelope_log)

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
        contents = get_log_contents(self.mcs_envelope_log)
        self.marked_mcs_envelope_logs = len(contents)

    def check_marked_mcs_envelope_log_contains(self, string_to_contain):
        contents = get_log_contents(self.mcs_envelope_log)

        contents = contents[self.marked_mcs_envelope_logs:]

        if string_to_contain not in contents:
            self.dump_mcs_envelope_log()
            raise AssertionError(f"MCS Envelope log did not contain: {string_to_contain}")

    def dump_mcsrouter_log(self):
        self.dump_log(self.mcs_router_log)

    def check_mcsrouter_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.mcs_router_log, "MCS Router")
        logger.info(self.mcs_router_log)

    def check_mcsrouter_log_contains_in_order(self, *args):
        log_contains_in_order(self.mcs_router_log, "MCS Router", args)

    def check_mcsrouter_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.mcs_router_log, "MCS Router")

    def check_mcsrouter_does_not_contain_critical_exceptions(self):
        if os.path.exists(self.mcs_router_log):
            self.check_log_does_not_contain("Caught exception at top-level;", self.mcs_router_log, "MCS router")

    def mark_mcsrouter_log(self):
        contents = get_log_contents(self.mcs_router_log)
        self.marked_mcsrouter_logs = len(contents)
        logger.info(f"mcsrouter log marked at line: {self.marked_mcsrouter_logs}")

    def check_marked_mcsrouter_log_contains(self, string_to_contain):
        contents = get_log_contents(self.mcs_router_log)

        contents = contents[self.marked_mcsrouter_logs:]

        if string_to_contain not in contents:
            self.dump_mcsrouter_log()
            raise AssertionError(f"MCS Router log did not contain: {string_to_contain}")

    def check_marked_mcsrouter_log_does_not_contain(self, string_not_to_contain):
        contents = get_log_contents(self.mcs_router_log)
        contents = contents[self.marked_mcsrouter_logs:]

        if string_not_to_contain in contents:
            self.dump_mcsrouter_log()
            raise AssertionError(f"MCS Router log contained: {string_not_to_contain}")

    def check_marked_mcsrouter_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = get_log_contents(self.mcs_router_log)

        contents = contents[self.marked_mcsrouter_logs:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                f"McsRouter Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                f"{expected_occurrence} times")

    # Management Agent Log Utils
    def dump_managementagent_log(self):
        self.dump_log(self.managementagent_log)

    def mark_managementagent_log(self):
        contents = get_log_contents(self.managementagent_log)
        self.marked_managementagent_log = len(contents)

    def check_management_agent_log_contains(self, string_to_contain):
        log = self.managementagent_log
        self.check_log_contains(string_to_contain, log, "ManagementAgent")
        logger.info(log)

    def check_marked_managementagent_log_contains(self, string_to_contain):
        contents = get_log_contents(self.managementagent_log)

        contents = contents[self.marked_managementagent_log:]

        if string_to_contain not in contents:
            self.dump_managementagent_log()
            raise AssertionError(f"Marked managementagent log did not contain: {string_to_contain}")

    def check_managementagent_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = get_log_contents(self.managementagent_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"managementagent Log Contains: \"{string_to_contain}\" - {num_occurrences} "
                                 f"times not the requested {expected_occurrence} times")

    def check_marked_managementagent_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = get_log_contents(self.managementagent_log)

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
        contents = get_log_contents(self.update_scheduler_log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"updatescheduler Log Contains: \"{string_to_contain}\" - {num_occurrences} times not "
                                 f"the requested {expected_occurrence} times")

    def mark_update_scheduler_log(self):
        contents = get_log_contents(self.update_scheduler_log)
        self.marked_update_scheduler_logs = len(contents)

    def check_marked_update_scheduler_log_contains(self, string_to_contain):
        update_scheduler_log = self.update_scheduler_log
        contents = get_log_contents(update_scheduler_log)

        contents = contents[self.marked_update_scheduler_logs:]

        if string_to_contain not in contents:
            self.dump_log(update_scheduler_log)
            raise AssertionError(f"Marked update scheduler log did not contain: {string_to_contain}")

    # AV Plugin Log Utils
    def mark_av_log(self):
        contents = get_log_contents(self.av_log)
        self.marked_av_log = len(contents)

    def check_marked_av_log_contains(self, string_to_contain):
        av_log = self.av_log
        contents = get_log_contents(av_log)

        contents = contents[self.marked_av_log:]

        if string_to_contain not in contents:
            self.dump_log(av_log)
            raise AssertionError(f"av.log log did not contain: {string_to_contain}")

    def mark_sophos_threat_detector_log(self):
        contents = get_log_contents(self.sophos_threat_detector_log)
        self.marked_sophos_threat_detector_log = len(contents)

    def check_marked_sophos_threat_detector_log_contains(self, string_to_contain):
        sophos_threat_detector_log = self.sophos_threat_detector_log
        contents = get_log_contents(sophos_threat_detector_log)

        contents = contents[self.marked_sophos_threat_detector_log:]

        if string_to_contain not in contents:
            self.dump_log(sophos_threat_detector_log)
            raise AssertionError(f"sophos_threat_detector.log log did not contain: {string_to_contain}")

    # EDR Log Utils
    def check_edr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.edr_log, "EDR")

    def check_edr_log_does_not_contain(self, string_to_not_contain):
        log = self.edr_log
        self.check_log_does_not_contain(string_to_not_contain, log, "EDR")

    def check_edr_log_contains_string_n_times(self, string_to_contain, expected_occurrence):

        log = self.edr_log
        contents = get_log_contents(log)

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(f"EDR Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                                 f"{expected_occurrence} times")

    def mark_edr_log(self):
        contents = get_log_contents(self.edr_log)
        self.marked_edr_log = len(contents)

    def check_marked_edr_log_contains(self, string_to_contain):
        contents = get_log_contents(self.edr_log)

        contents = contents[self.marked_edr_log:]

        if string_to_contain not in contents:
            raise AssertionError(f"EDR did not contain: {string_to_contain}")

    def mark_edr_osquery_log(self):
        contents = get_log_contents(self.edr_osquery_log)
        self.marked_edr_osquery_log = len(contents)

    def check_marked_edr_osquery_log_contains(self, string_to_contain):
        contents = get_log_contents(self.edr_osquery_log)

        contents = contents[self.marked_edr_osquery_log:]

        if string_to_contain not in contents:
            raise AssertionError(f"EDR did not contain: {string_to_contain}")

    def dump_livequery_log(self):
        self.dump_log(self.livequery_log)

    def check_livequery_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.livequery_log, "Livequery")

    def mark_livequery_log(self, expect_log_exists=True):
        livequery_log = self.livequery_log
        if not expect_log_exists:
            if not os.path.isfile(livequery_log):
                self.marked_livequery_log = 0
                return
        contents = get_log_contents(livequery_log)
        self.marked_livequery_log = len(contents)
        logger.info(f"livequery log marked at line: {self.marked_livequery_log}")

    def check_marked_livequery_log_contains(self, string_to_contain):
        contents = get_log_contents(self.livequery_log)

        contents = contents[self.marked_livequery_log:]

        if string_to_contain not in contents:
            self.dump_livequery_log()
            raise AssertionError(f"Marked livequery log did not contain: {string_to_contain}")

    def check_marked_livequery_log_contains_string_n_times(self, string_to_contain, expected_occurrence):
        contents = get_log_contents(self.livequery_log)

        contents = contents[self.marked_livequery_log:]

        num_occurrences = self.get_number_of_occurrences_of_substring_in_string(contents, string_to_contain)
        if num_occurrences != int(expected_occurrence):
            raise AssertionError(
                f"Livequery Log Contains: \"{string_to_contain}\" - {num_occurrences} times not the requested "
                f"{expected_occurrence} times")

    # Event Journaler Log Utils
    def check_event_journaler_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.ej_log, "Journaler")

    # Live Response Log Utils
    def check_liveresponse_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.liveresponse_log, "Liveresponse")

    def check_sessions_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.sessions_log, "sessions")

    # MTR Log Utils
    def remove_mtr_plugin_log(self):
        if os.path.isfile(self.mdr_log):
            os.remove(self.mdr_log)

    def check_mdr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.mdr_log, "MDR")

    def check_mtr_log_does_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.mdr_log, "MTR Plugin Log")

    def check_mdr_log_contains_in_order(self, *args):
        log_contains_in_order(self.mdr_log, "MDR", args)

    def osquery_watcher_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.osquery_watcher_log, "osquery watcher log")

    def osquery_watcher_log_should_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.osquery_watcher_log, "osquery watcher log")

    def get_value_from_ini_file(self, key: str, file: str, section: str = "fake_section") -> str:
        config = configparser.ConfigParser()
        try:
            config.read(file)
        except configparser.MissingSectionHeaderError:
            with open(file) as f:
                file_content = f'[{section}]\n' + f.read()
                config = configparser.RawConfigParser()
                config.read_string(file_content)
        return config[section][key]

    def get_version_number_from_ini_file(self, file):
        with open(file) as ini_file:
            lines = ini_file.readlines()
            version_text = "PRODUCT_VERSION = "
            for line in lines:
                if version_text in line:
                    version_string = line[len(version_text):]
                    logger.info(f"Found version string: {version_string}")
                    return version_string.strip()
        logger.error("Version String Not Found")
        raise AssertionError("Version String Not Found")

    def version_number_in_ini_file_should_be(self, file, expected_version):
        actual_version = self.get_version_number_from_ini_file(file)
        if actual_version != expected_version:
            raise AssertionError(f"Expected version {actual_version} to be {expected_version}")

    def dump_all_processes(self):
        pstree = '/usr/bin/pstree'
        if os.path.isfile(pstree):
            logger.info(subprocess.check_output([pstree, '-ap'], stderr=subprocess.PIPE))
        else:
            logger.info(subprocess.check_output(['/bin/ps', '-elf'], stderr=subprocess.PIPE))
        try:
            top_path = [candidate for candidate in ['bin/top', '/usr/bin/top'] if os.path.isfile(candidate)][0]
            logger.info(subprocess.check_output([top_path, '-bHn1', '-o', '+%CPU', '-o', '+%MEM']))
        except Exception as ex:
            logger.warn(ex.message)

    def dump_push_server_log(self):
        server_log = os.path.join(self.tmp_path, "push_server_log.log")
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
