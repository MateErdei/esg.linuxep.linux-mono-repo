from datetime import datetime
from datetime import timedelta
import calendar
import os

RESOURCES_DIR = "/opt/test/inputs/test_scripts/resources"
SAV_POLICY_FILENAME = "SAV_Policy.xml"
SAV_POLICY_PATH = os.path.join(RESOURCES_DIR, SAV_POLICY_FILENAME)

def create_sav_policy_with_scheduled_scan(filename, timestamp):
    parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
    day = calendar.day_name[parsed_timestamp.weekday()].lower()
    parsed_timestamp = (parsed_timestamp + timedelta(minutes=2)).strftime("%H:%M:%S")

    sav_policy_builder = _SavPolicyBuilder("/opt/test/inputs/test_scripts/resources/SAV_Policy.xml", filename)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(parsed_timestamp)
    sav_policy_builder.send_sav_policy()

def create_sav_policy_with_multiple_scheduled_scans(filename, timestamp, no_of_scans=2):
    timestamp_builder = ""
    for scan in range(no_of_scans):
        parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
        day = calendar.day_name[parsed_timestamp.weekday()].lower()
        timestamp_builder += "<time>" + (parsed_timestamp + timedelta(minutes=2, seconds=scan)).strftime("%H:%M:%S") + "</time>\n\t\t\t"

    sav_policy_builder = _SavPolicyBuilder("/opt/test/inputs/test_scripts/resources/SAV_Policy_Configurable_Multiple_Scheduled_Scans.xml", filename)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(timestamp_builder)
    sav_policy_builder.send_sav_policy()


def create_badly_configured_sav_policy_time(filename):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("monday")
    sav_policy_builder.set_scheduled_scan_time("36:00:00")
    sav_policy_builder.send_sav_policy()

def create_badly_configured_sav_policy_day(filename):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("blernsday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.send_sav_policy()

def create_complete_sav_policy(filename):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("monday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.set_posix_exclusions(["*.glob", "globExample?.txt", "/stemexample/*"])
    sav_policy_builder.set_sophos_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3"])
    sav_policy_builder.set_user_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3", "exclusion4"])
    sav_policy_builder.send_sav_policy()


class _SavPolicyBuilder:
    def __init__(self, input_path, output_name):
        self.path = input_path
        self.output_path = "/opt/test/inputs/test_scripts/resources/" + output_name
        # Defaults
        self.replacement_map = {"{{allFiles}}": "false",
                                "{{excludeSophosDefined}}": "",
                                "{{excludeUserDefined}}": "",
                                "{{noExtensions}}": "true"}

    def send_sav_policy(self):
        content = self.get_sav_policy()
        with open(self.output_path, "w") as out:
            out.write(content)

    def get_sav_policy(self):
        with open(self.path, "r") as xml:
            policy = xml.read()
            for key, value in self.replacement_map.items():
                policy = policy.replace(key, value)
            return policy

    def set_scheduled_scan_day(self, day):
        self.replacement_map["{{day}}"] = day

    def set_scheduled_scan_time(self, time):
        self.replacement_map["{{scheduledScanTime}}"] = time

    def set_posix_exclusions(self, exclusions):
        self.replacement_map["{{posixExclusions}}"] = self._create_tagged_lines(exclusions, "filePath")

    def set_sophos_defined_extension_exclusions(self, extension_exclusions):
        self.replacement_map["{{excludeSophosDefined}}"] = self._create_tagged_lines(extension_exclusions, "extension")

    def set_user_defined_extension_exclusions(self, extension_exclusions):
        self.replacement_map["{{excludeUserDefined}}"] = self._create_tagged_lines(extension_exclusions, "extension")

    def set_on_demand_all_files_flag(self, all_files_flag):
        self.replacement_map["{{allFiles}}"] = all_files_flag

    def set_on_demand_no_extensions_flag(self, no_extensions_flag):
        self.replacement_map["{{noExtensions}}"] = no_extensions_flag

    @staticmethod
    def _create_tagged_lines(object_list, tag):
        line = ""
        for thing in object_list:
            line += "<" + tag + ">" + thing + "</" + tag + ">\n"
        return line

