import os
import glob
import subprocess
import dateutil.parser
from robot.api import logger
import robot.libraries.BuiltIn


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
                logger.info("{} log contains {}".format(log_name, string))
            index = index + len(string)
        else:
            raise AssertionError("Remainder of {} log doesn't contain {}".format(log_name, string))

def get_variable(varName, defaultValue=None):
    try:
        return robot.libraries.BuiltIn.BuiltIn().get_variable_value("${%s}" % varName)
    except robot.libraries.BuiltIn.RobotNotRunningError:
        return os.environ.get(varName, defaultValue)

class LogUtils(object):
    def __init__(self):
        self.tmp_path = os.path.join(".", "tmp")

        # Get Robot variables.
        self.install_path = get_variable("SOPHOS_INSTALL", os.path.join("/", "opt", "sophos-spl"))
        self.router_path = get_variable("MCS_DIR", os.path.join(self.install_path, "base", "mcs"))
        self.base_logs_dir = get_variable("BASE_LOGS_DIR", os.path.join(self.install_path, "logs", "base"))
        self.thin_install_log = os.path.join(self.tmp_path, "thin_installer", "ThinInstaller.log")
        self.suldownloader_log = os.path.join(self.base_logs_dir, "suldownloader.log")
        self.update_scheduler_log = os.path.join(self.base_logs_dir, "sophosspl", "updatescheduler.log")
        self.register_log = os.path.join(self.base_logs_dir, "register_central.log")
        self.mdr_log = os.path.join(self.install_path, "plugins", "mtr", "log", "mtr.log")
        self.edr_log = os.path.join(self.install_path, "plugins", "edr", "log", "edr.log")
        self.livequery_log = os.path.join(self.install_path, "plugins", "edr", "log", "livequery.log")
        self.liveresponse_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "liveresponse.log")
        self.sessions_log = os.path.join(self.install_path, "plugins", "liveresponse", "log", "sessions.log")
        self.osquery_watcher_log = os.path.join(self.install_path, "plugins", "mtr", "dbos", "data", "logs", "osquery.watcher.log")
        self.cloud_server_log = os.path.join(self.tmp_path, "cloudServer.log")
        self.marked_mcsrouter_logs = 0
        self.marked_mcs_envelope_logs = 0
        self.marked_watchdog_logs = 0
        self.marked_managementagent_log = 0

    def log_contains_in_order(self, log_location, log_name, args, log_finds=True):
        return log_contains_in_order(log_location, log_name, args, log_finds)

    def get_register_central_log(self):
        return self.register_log

    def dump_log(self, filename):
        if os.path.isfile(filename):
            try:
                with open(filename, "r") as f:
                    logger.info("file: " + str(filename))
                    logger.info(''.join(f.readlines()))
            except Exception as e:
                logger.info("Failed to read file: {}".format(e))
        else:
            logger.info("File does not exist")

    def dump_cloud_server_log(self):
        server_log = self.cloud_server_log
        self.dump_log(server_log)

    def dump_push_server_log(self):
        server_log = os.path.join(self.tmp_path, "push_server_log.log")
        self.dump_log(server_log)

    def check_log_contains(self, string_to_contain, pathToLog, log_name):
        if not (os.path.isfile(pathToLog)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, pathToLog))

        contents = get_log_contents(pathToLog)
        if string_to_contain not in contents:
            self.dump_log(pathToLog)
            raise AssertionError("{} Log at \"{}\" does not contain: {}".format(log_name, pathToLog, string_to_contain))

    def check_log_and_return_nth_occurence_between_strings(self, string_to_contain_start, string_to_contain_end, pathToLog,
                                                           occurs=1):
        if not (os.path.isfile(pathToLog)):
            raise AssertionError("Log file {} does not exist".format(pathToLog))

        occurs_int = int(occurs)

        contents = get_log_contents(pathToLog)

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

        self.dump_log(pathToLog)
        raise AssertionError(
            "Log at \"{}\" does not contain following string pair {} - {} times : {}".format(pathToLog,
                                                                                             string_to_contain_start,
                                                                                             string_to_contain_end,
                                                                                             occurs))

    def check_mcs_envelope_for_status_same_in_at_least_one_of_the_following_statuses_sent_with_the_prior_being_noref(self, *status_numbers):
        # status_numbers should be integers
        # will check all statuses in order given to see if they are res=same
        # will pass when res=same is found
        # will fail on a status where res is neither same or noref
        # the purpose of this function is to solve a race condition in a flakey test
        statuses = []
        for status_number in status_numbers:
            try:
                statuses.append(self.check_log_and_return_nth_occurence_between_strings("<status><appId>ALC</appId>", "</status>", "{}/logs/base/sophosspl/mcs_envelope.log".format(self.install_path),  int(status_number)))
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


    def check_log_does_not_contain(self, string_not_to_contain, pathToLog, log_name):
        if not (os.path.isfile(pathToLog)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, pathToLog))

        contents = get_log_contents(pathToLog)
        if string_not_to_contain in contents:
            self.dump_log(pathToLog)
            raise AssertionError("{} Log at \"{}\" does contain: {}".format(log_name, pathToLog, string_not_to_contain))

    def check_log_contains_more_than_one_pair_of_strings_in_order(self, stringOne, stringTwo, pathToLog, log_name):
        logger.info("Strings are {} {}".format(stringOne, stringTwo))
        if not (os.path.isfile(pathToLog)):
            raise AssertionError("Log file {} at location {} does not exist ".format(log_name, pathToLog))

        contents = get_log_contents(pathToLog)

        first = True
        index2 = 0
        while True:
            index1 = contents.find(stringOne, index2)
            logger.info("Index1 = {}".format(index1))
            if index1 == -1:
                if first:
                    raise AssertionError(
                        "{} Log at \"{}\" does contain first string in pair at all: {} - {}".format(log_name, pathToLog,
                                                                                                    stringOne,
                                                                                                    stringTwo))
                else:
                    break
            index1 = index1 + len(stringOne)
            index2 = contents.find(stringTwo, index1)
            logger.info("Index2 = {}".format(index2))
            if index2 == -1:
                self.dump_log(pathToLog)
                raise AssertionError(
                    "{} Log at \"{}\" does contain pair of strings in order: {} - {}".format(log_name, pathToLog,
                                                                                             stringOne, stringTwo))
            index2 = index2 + len(stringTwo)
            first = False

    def check_log_contains_data_from_multisend(self, log_root, subscriber, publisher_num, data_items, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            data = []
            for datum in range(0, int(data_items)):
                data.append(
                    "Sub {} received message: ['{}', 'Data From publisher {} Num {}']".format(subscriber, channel_name,
                                                                                              publisher_id, datum))
            log_contains_in_order(os.path.join(log_root, "fake_multi_subscriber.log")
                                  , "Fake Multi-Subscriber Log"
                                  , data
                                  , False)

    def check_multiple_logs_contains_data_from_multisend(self, log_root, publisher_num, subscriber_num, data_items,
                                                         channel_name_log, channel_name):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                data = []
                for datum in range(0, int(data_items)):
                    data.append("Sub Subscriber_{}_{} received message: ['{}', 'Data From publisher {} Num {}']".format(
                        subscriber_id, channel_name_log, channel_name, publisher_id, datum))
                log_contains_in_order(
                    os.path.join(log_root, "Subscriber_{}_{}.log".format(subscriber_id, channel_name_log))
                    , "Fake Subscriber {} {} Log".format(subscriber_id, channel_name_log)
                    , data
                    , False)

    def check_multiple_logs_do_not_contain_data_from_multisend(self, log_root, publisher_num, subscriber_num,
                                                               channel_name_listened_to, channel_name_not_listened_to):
        for publisher_id in range(0, int(publisher_num)):
            for subscriber_id in range(0, int(subscriber_num)):
                self.check_log_does_not_contain(
                    "Sub Subscriber_{}_{} received message: ['{}', 'Data From publisher {} Num".format(subscriber_id,
                                                                                                       channel_name_listened_to,
                                                                                                       channel_name_not_listened_to,
                                                                                                       publisher_id)
                    , os.path.join(log_root, "Subscriber_{}_{}.log".format(subscriber_id, channel_name_listened_to))
                    , "Fake Subscriber {} {} Log".format(subscriber_id, channel_name_listened_to))

    def check_all_product_logs_do_not_contain_string(self, string_to_find):
        search_list = ["logs/base/*.log*", "logs/base/sophosspl/*.log*", "plugins/*/log/*.log*"]
        glob_search_pattern = [os.path.join(self.install_path, search_entry) for search_entry in search_list]
        combinedfiles = [glob.glob(search_pattern) for search_pattern in glob_search_pattern]
        flat_files = [item for sublist in combinedfiles for item in sublist]
        list_of_logs_containing_string = []
        for filepath in flat_files:
            num_occurence = self.get_number_of_occurences_of_substring_in_log(filepath, string_to_find)
            if num_occurence > 0:
                list_of_logs_containing_string.append("{} - {} times".format(filepath, num_occurence))
        if list_of_logs_containing_string:
            raise AssertionError("These program logs contain {}:\n {}".format(string_to_find, list_of_logs_containing_string))

    def check_all_product_logs_do_not_contain_error(self):
        self.check_all_product_logs_do_not_contain_string("ERROR")

    def check_all_product_logs_do_not_contain_critical(self):
        self.check_all_product_logs_do_not_contain_string("CRITICAL")

    def get_number_of_occurences_of_substring_in_log(self, log_location, substring):
        contents = get_log_contents(log_location)
        return self.get_number_of_occurences_of_substring_in_string(contents, substring)

    def get_number_of_occurences_of_substring_in_string(self, string, substring, use_regex=False):
        if use_regex:
            return self.get_number_of_occurences_of_regex_in_string(string, substring)
        count = 0
        index = 0
        while True:
            index = string.find(substring, index)
            if index == -1:
                break
            index += len(substring)
            count += 1
        return count

    #require that special characters are escaped with '\' [ /, +, *, ., (, ) etc ]
    def get_number_of_occurences_of_regex_in_string(self, string, reg_expresion_str):
        import re
        reg_expression = re.compile(reg_expresion_str)
        log_occurrences = reg_expression.findall(string)
        return len(log_occurrences)


    def check_string_matching_regex_in_file(self, file_path, reg_expression_str):
        if not os.path.exists(file_path):
            raise AssertionError("File not found '{}'".format(file_path))
        if self.get_number_of_occurences_of_regex_in_string(get_log_contents(file_path), reg_expression_str) < 1:
            self.dump_log(file_path)
            raise AssertionError(
                "The file: '{}', did not have any lines match the regex: '{}'".format(file_path, reg_expression_str))

    def get_log_line(self,string_to_contain, path_to_log):
        if not (os.path.isfile(path_to_log)):
            raise AssertionError("Log file {} does not exist ".format(path_to_log))

        with open(path_to_log, "r") as log:
            contents = log.readlines()

        for line in contents:
            if string_to_contain in line:
                return line

        raise AssertionError("Log at \"{}\" does not contain: {}".format(path_to_log, string_to_contain))

    def get_timestamp_of_log_line(self,string_to_contain, path_to_log):
        line = self.get_log_line(string_to_contain, path_to_log)
        timestamp = line.split("[")[1].split("]")[0]

        return dateutil.parser.isoparse(timestamp)

    def get_time_difference_between_two_log_lines(self, string_to_contain1, string_to_contain2, path_to_log):
        timestamp1 = self.get_timestamp_of_log_line(string_to_contain1, path_to_log)
        timestamp2 = self.get_timestamp_of_log_line(string_to_contain2, path_to_log)

        difference = timestamp2 - timestamp1

        return difference.total_seconds()

    def mark_expected_error_in_log(self, log_location, error_message):
        error_string = "ERROR"
        mark_string = "expected-error"
        index = 0
        contents = get_log_contents(log_location)
        while True:
            index = contents.find(error_message, index)
            if index == -1:
                break
            error_index = contents.rfind(error_string, index - 40, index)
            contents = contents[:error_index] + mark_string + contents[error_index + len(error_string):]
            index += len(error_message) + len(error_string) - len(mark_string)
        with open(log_location, "w") as log:
            log.write(contents)

    def mcs_router_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")

    def dump_mcsrouter_log(self):
        mcsrouter_log = self.mcs_router_log()
        self.dump_log(mcsrouter_log)

    def check_mcsrouter_log_contains_in_order(self, *args):
        log_contains_in_order(self.mcs_router_log(), "MCS Router", args)

    def mark_mcsrouter_log(self):
        mcsrouter_log = self.mcs_router_log()
        contents = get_log_contents(mcsrouter_log)
        self.marked_mcsrouter_logs = len(contents)

    def check_marked_mcsrouter_log_contains(self, string_to_contain):
        mcsrouter_log = self.mcs_router_log()
        contents = get_log_contents(mcsrouter_log)

        contents = contents[self.marked_mcsrouter_logs:]

        if string_to_contain not in contents:
            self.dump_mcsrouter_log()
            raise AssertionError("MCS Router log did not contain: " + string_to_contain)

    def check_marked_mcsrouter_log_contains_string_n_times(self, string_to_contain, expected_occurence):
        mcsrouter_log = os.path.join(self.base_logs_dir, "sophosspl", "mcsrouter.log")
        contents = get_log_contents(mcsrouter_log)

        contents = contents[self.marked_mcsrouter_logs:]

        num_occurences = self.get_number_of_occurences_of_substring_in_string(contents, string_to_contain)
        if num_occurences != int(expected_occurence):
            raise AssertionError(
                "McsRouter Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain,
                                                                                              num_occurences,
                                                                                              expected_occurence))
    def check_mcsrouter_log_contains(self, string_to_contain):
        mcsrouter_log = self.mcs_router_log()
        self.check_log_contains(string_to_contain, mcsrouter_log, "MCS Router")
        logger.info(mcsrouter_log)

    def check_mcsrouter_log_does_not_contain(self, string_to_not_contain):
        mcsrouter_log = self.mcs_router_log()
        self.check_log_does_not_contain(string_to_not_contain, mcsrouter_log, "MCS Router")

    def check_mcsrouter_does_not_contain_critical_exceptions(self):
        if os.path.exists(self.mcs_router_log()):
            self.check_log_does_not_contain("Caught exception at top-level;", self.mcs_router_log(), "MCS router")

    def dump_thininstaller_log(self):
        self.dump_log(self.thin_install_log)

    def check_thininstaller_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_does_not_contain(self, string_not_to_contain):
        self.check_log_does_not_contain(string_not_to_contain, self.thin_install_log, "Thin Installer")

    def check_thininstaller_log_contains_in_order(self, *args):
        log_contains_in_order(self.thin_install_log, "Thin installer", args)

    def remove_thininstaller_log(self, *args):
        if os.path.isfile(self.thin_install_log):
            os.remove(self.thin_install_log)

    def check_livequery_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.livequery_log, "Livequery")

    def dump_livequery_log(self):
        self.dump_log(self.livequery_log)

    def check_liveresponse_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.liveresponse_log, "Liveresponse")

    def check_sessions_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.sessions_log, "sessions")

    def check_suldownloader_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.suldownloader_log, "Suldownloader")

    def osquery_watcher_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.osquery_watcher_log, "osquery watcher log")

    def osquery_watcher_log_should_not_contain(self, string_to_not_contain):
        self.check_log_does_not_contain(string_to_not_contain, self.osquery_watcher_log, "osquery watcher log")

    def cloud_server_log_should_contain(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.cloud_server_log, "cloud server log")

    def cloud_server_log_should_not_contain(self, string_to_contain):
        self.check_log_does_not_contain(string_to_contain, self.cloud_server_log, "cloud server log")

    def remove_mtr_plugin_log(self):
        if os.path.isfile(self.mdr_log):
            os.remove(self.mdr_log)

    def check_suldownloader_log_contains_in_order(self, *args):
        log_contains_in_order(self.suldownloader_log, "Suldownloader", args)


    def check_log_contains_in_order(self, log_path, *args):
        log_contains_in_order(log_path, log_path, args)

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
        log_contains_in_order(self.register_log, "Register Central", args)

    def check_mdr_log_contains_in_order(self, *args):
        log_contains_in_order(self.mdr_log, "MDR", args)

    def check_register_central_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.register_log, "Register Central")

    def check_edr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.edr_log, "EDR")

    def check_mdr_log_contains(self, string_to_contain):
        self.check_log_contains(string_to_contain, self.mdr_log, "MDR")

    def mcs_envelope_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")

    def watchdog_log(self):
        return os.path.join(self.base_logs_dir, "watchdog.log")

    def managementagent_log(self):
        return os.path.join(self.base_logs_dir, "sophosspl", "sophos_managementagent.log")

    def dump_mcs_envelope_log(self):
        mcs_envelope_log = self.mcs_envelope_log()
        self.dump_log(mcs_envelope_log)

    def dump_watchdog_log(self):
        watchdog_log = self.watchdog_log()
        self.dump_log(watchdog_log)

    def dump_managementagent_log(self):
        managementagent_log = self.managementagent_log()
        self.dump_log(managementagent_log)

    def mark_mcs_envelope_log(self):
        mcs_envelope_log = self.mcs_envelope_log()
        contents = get_log_contents(mcs_envelope_log)
        self.marked_mcs_envelope_logs = len(contents)

    def mark_watchdog_log(self):
        watchdog_log = self.watchdog_log()
        contents = get_log_contents(watchdog_log)
        self.marked_watchdog_logs = len(contents)

    def mark_managementagent_log(self):
        managementagent_log = self.managementagent_log()
        contents = get_log_contents(managementagent_log)
        self.marked_managementagent_logs = len(contents)

    def check_marked_mcs_envelope_log_contains(self, string_to_contain):
        mcs_envelope_log = self.mcs_envelope_log()
        contents = get_log_contents(mcs_envelope_log)

        contents = contents[self.marked_mcs_envelope_logs:]

        if string_to_contain not in contents:
            self.dump_mcs_envelope_log()
            raise AssertionError("MCS Envelope log did not contain: " + string_to_contain)

    def check_marked_watchdog_log_contains(self, string_to_contain):
        watchdog_log = self.watchdog_log()
        contents = get_log_contents(watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain not in contents:
            self.dump_watchdog_log()
            raise AssertionError("Marked watchdog log did not contain: " + string_to_contain)

    def check_marked_watchdog_log_does_not_contain(self, string_to_contain):
        watchdog_log = self.watchdog_log()
        contents = get_log_contents(watchdog_log)

        contents = contents[self.marked_watchdog_logs:]

        if string_to_contain in contents:
            self.dump_watchdog_log()
            raise AssertionError("Marked watchdog log did not contain: " + string_to_contain)

    def check_marked_managementagent_log_contains(self, string_to_contain):
        managementagent_log = self.managementagent_log()
        contents = get_log_contents(managementagent_log)

        contents = contents[self.marked_managementagent_logs:]

        if string_to_contain not in contents:
            self.dump_managementagent_log()
            raise AssertionError("Marked managementagent log did not contain: " + string_to_contain)

    def check_log_contains_string_n_times(self, log_path, log_name, string_to_contain, expected_occurence):
        contents = get_log_contents(log_path)

        num_occurences = self.get_number_of_occurences_of_substring_in_string(contents, string_to_contain)
        if num_occurences != int(expected_occurence):
            raise AssertionError(
                "{} Contains: \"{}\" - {} times not the requested {} times".format(log_name,
                                                                                   string_to_contain,
                                                                                   num_occurences,
                                                                                   expected_occurence))

    def check_managementagent_log_contains_string_n_times(self, string_to_contain, expected_occurence):

        managementagent_log = self.managementagent_log()
        contents = get_log_contents(managementagent_log)

        num_occurences = self.get_number_of_occurences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurences != int(expected_occurence):
            raise AssertionError("managementagent Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain, num_occurences, expected_occurence))

    def check_updatescheduler_log_contains_string_n_times(self, string_to_contain, expected_occurence):

        updatescheduler_log = self.update_scheduler_log
        contents = get_log_contents(updatescheduler_log)

        num_occurences = self.get_number_of_occurences_of_substring_in_string(contents, string_to_contain, use_regex=False)
        if num_occurences != int(expected_occurence):
            raise AssertionError("updateschedulerg Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain, num_occurences, expected_occurence))


    def check_mcs_envelope_log_contains_regex_string_n_times(self, string_to_contain, expected_occurence):
        self.check_mcs_envelope_log_contains_string_n_times(string_to_contain, expected_occurence, True)

    def check_mcs_envelope_log_contains_string_n_times(self, string_to_contain, expected_occurence, use_regex=False):
        mcs_envelope_log = os.path.join(self.base_logs_dir, "sophosspl", "mcs_envelope.log")
        contents = get_log_contents(mcs_envelope_log)

        num_occurences = self.get_number_of_occurences_of_substring_in_string(contents, string_to_contain, use_regex)
        if num_occurences != int(expected_occurence):
            raise AssertionError("mcs_envelope Log Contains: \"{}\" - {} times not the requested {} times".format(string_to_contain, num_occurences, expected_occurence))

    def check_updatescheduler_log_contains(self, string_to_contain):
        updatescheduler_log = self.update_scheduler_log
        self.check_log_contains(string_to_contain, updatescheduler_log, "UpdateScheduler")
        logger.info(updatescheduler_log)



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

        log_contains_in_order(mcsrouter_log, "mcs_router", targets)

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
        log_contains_in_order(mcs_envelope_log, "MCS Envelope", args)

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

    def dump_all_processes(self):
        pstree = '/usr/bin/pstree'
        if os.path.isfile(pstree):
            logger.info(subprocess.check_output([pstree, '-ap'], stderr=subprocess.PIPE))
        else:
            logger.info(subprocess.check_output(['/bin/ps', '-elf'], stderr=subprocess.PIPE))
        try:
            toppath = [candidate for candidate in ['bin/top', '/usr/bin/top'] if os.path.isfile(candidate)][0]
            logger.info(subprocess.check_output([toppath, '-bHn1', '-o', '+%CPU', '-o', '+%MEM']))
        except Exception as ex:
            logger.warn(ex.message)

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

    def all_should_be_equal(self, *args):
        assert len(args) > 1, "Error: should have more than 1 argument"
        master = args[0]
        all_equal = True
        for arg in args[1:]:
            if arg != master:
                raise AssertionError("Not all items are equal in: {}".format(args))
