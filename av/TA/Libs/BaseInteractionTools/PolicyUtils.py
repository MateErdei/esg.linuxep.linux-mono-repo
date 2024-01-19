# Copyright 2020-2023 Sophos Limited. All rights reserved.

from datetime import datetime
from datetime import timedelta
import calendar
import json
import os
import time
import uuid

try:
    from .. import ExclusionHelper
except ImportError:
    try:
        from Libs import ExclusionHelper
    except ImportError:
        import ExclusionHelper

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger("PluginUtils")


RESOURCES_DIR = "/opt/test/inputs/test_scripts/resources"
SAV_POLICY_FILENAME = "SAV_Policy_Template.xml"
SAV_POLICY_PATH = os.path.join(RESOURCES_DIR, SAV_POLICY_FILENAME)
FIXED_SAV_POLICY_PATH = os.path.join(RESOURCES_DIR, "sav_policy", "SAV_Policy_Fixed_Exclusions.xml")
SAV_POLICY_TEMPLATE_PATH = os.path.join(RESOURCES_DIR, SAV_POLICY_FILENAME)
CORE_POLICY_TEMPLATE_PATH = os.path.join(RESOURCES_DIR, "core_policy", "CORE-36_template.xml")
CORC_POLICY_TEMPLATE_PATH = os.path.join(RESOURCES_DIR, "corc_policy", "corc_policy_template.xml")
ALC_POLICY_TEMPLATE_PATH = os.path.join(RESOURCES_DIR, "alc_policy", "template", "ALC_Template.xml")

def create_sav_policy_with_scheduled_scan(filename, timestamp):
    parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
    scan_time = parsed_timestamp + timedelta(seconds=20)
    day = calendar.day_name[scan_time.weekday()].lower()
    parsed_timestamp = scan_time.strftime("%H:%M:%S")

    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(parsed_timestamp)
    sav_policy_builder.set_posix_exclusions(["*/test_scripts/*", "*excluded*"])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.send_sav_policy()


def create_sav_policy_with_no_scheduled_scan(filename, exclusion_list=["*.glob", "globExample?.txt", "/stemexample/*"],
                                             on_access_enabled=False, revId=None):
    sav_policy_builder = _SavPolicyBuilder(os.path.join(RESOURCES_DIR, "SAV_Policy_No_Scans.xml"), filename)
    if on_access_enabled:
        sav_policy_builder.set_on_access_on()
    sav_policy_builder.set_posix_exclusions(exclusion_list)
    sav_policy_builder.set_sophos_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3"])
    sav_policy_builder.set_user_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3", "exclusion4"])
    sav_policy_builder.set_pua_detection("true")
    if revId:
        sav_policy_builder.set_revision_id(revId)
    else:
        sav_policy_builder.set_revision_id(str(uuid.uuid4()))
    sav_policy_builder.send_sav_policy()


def create_sav_policy_with_scheduled_scan_and_on_access_enabled(filename, timestamp):
    parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
    scan_time = parsed_timestamp + timedelta(seconds=20)
    day = calendar.day_name[scan_time.weekday()].lower()
    parsed_timestamp = scan_time.strftime("%H:%M:%S")

    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(parsed_timestamp)
    sav_policy_builder.set_posix_exclusions(["*/test_scripts/*", "*excluded*"])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.set_on_access_on()
    sav_policy_builder.send_sav_policy()


def create_sav_policy_with_scheduled_scan_and_pua_detection_disabled(filename, timestamp):
    parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
    scan_time = parsed_timestamp + timedelta(seconds=20)
    day = calendar.day_name[scan_time.weekday()].lower()
    parsed_timestamp = scan_time.strftime("%H:%M:%S")

    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(parsed_timestamp)
    sav_policy_builder.set_posix_exclusions(["*/test_scripts/*", "*excluded*"])
    sav_policy_builder.set_pua_detection("false")
    sav_policy_builder.send_sav_policy()


def create_sav_policy_with_on_access_enabled(filename):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("sunday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.set_posix_exclusions(["/mnt/", "/uk-filer5/", "*excluded*", "/opt/test/inputs/test_scripts/", ])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.set_on_access_on()
    sav_policy_builder.send_sav_policy()


def create_sav_policy_with_on_access_enabled_and_pua_allowed(filename, allowed_pua):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("sunday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.set_posix_exclusions(["/mnt/", "/uk-filer5/", "*excluded*", "/opt/test/inputs/test_scripts/", ])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.set_allowed_pua(allowed_pua)
    sav_policy_builder.set_on_access_on()
    sav_policy_builder.send_sav_policy()

def create_sav_policy_with_multiple_scheduled_scans(filename, timestamp, no_of_scans=2):
    timestamp_builder = ""
    parsed_timestamp = datetime.strptime(timestamp, "%y-%m-%d %H:%M:%S")
    # Cover today and tomorrow, to handle time wrap around
    day = parsed_timestamp.weekday()
    days = [
        calendar.day_name[day].lower(),
        calendar.day_name[(day+1) % 7].lower()
    ]
    assert no_of_scans >= 1

    for scan in range(no_of_scans):
        delta = timedelta(seconds=20+scan*20)
        timestamp_builder += "<time>" + (parsed_timestamp + delta).strftime("%H:%M:%S") + "</time>\n\t\t\t"

    sav_policy_builder = _SavPolicyBuilder(RESOURCES_DIR+"/SAV_Policy_Configurable_Multiple_Scheduled_Scans.xml", filename)
    day = "</day><day>".join(days)
    sav_policy_builder.set_scheduled_scan_day(day)
    sav_policy_builder.set_scheduled_scan_time(timestamp_builder)
    sav_policy_builder.set_posix_exclusions(["*/test_scripts/*", "*excluded*"])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.send_sav_policy()


def create_complete_sav_policy(filename):
    sav_policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("monday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.set_posix_exclusions(["*.glob", "globExample?.txt", "/stemexample/*"])
    sav_policy_builder.set_sophos_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3"])
    sav_policy_builder.set_user_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3", "exclusion4"])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.send_sav_policy()


def get_complete_sav_policy(
        exclusion_list=None,
        on_access_enabled=False,
        on_access_on_read=True,
        on_access_on_write=True
    ):
    if exclusion_list is None:
        exclusion_list = ["*.glob", "globExample?.txt", "/stemexample/*"]
    policy_builder = _SavPolicyBuilder(SAV_POLICY_PATH, None)
    policy_builder.set_scheduled_scan_day("monday")
    policy_builder.set_scheduled_scan_time("11:00:00")
    if on_access_enabled:
        policy_builder.set_on_access_on()
    policy_builder.set_on_access_on_read(on_access_on_read, "fileRead")
    policy_builder.set_on_access_on_write(on_access_on_write, "fileWrite")
    policy_builder.set_posix_exclusions(exclusion_list)
    policy_builder.set_sophos_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3"])
    policy_builder.set_user_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3", "exclusion4"])
    policy_builder.set_pua_detection("true")
    policy_builder.set_revision_id(str(uuid.uuid4()))
    return policy_builder.get_sav_policy()


def get_complete_core_policy(
        exclusion_list=None,
        on_access_enabled=False,
        on_access_on_read=True,
        on_access_on_write=True,
        ml_enabled=True):
    if exclusion_list is None:
        exclusion_list = ["*.glob", "globExample?.txt", "/stemexample/*"]
    policy_builder = _SavPolicyBuilder(CORE_POLICY_TEMPLATE_PATH, None)
    policy_builder.set_on_access(on_access_enabled)
    policy_builder.set_on_access_on_read(on_access_on_read, "onRead")
    policy_builder.set_on_access_on_write(on_access_on_write, "onWrite")
    policy_builder.set_posix_exclusions(exclusion_list)
    policy_builder.add_replacement('{{excludeRemoteFiles}}', 'false')
    policy_builder.add_replacement('{{machineLearningEnabled}}', 'true' if ml_enabled else 'false')

    policy_builder.set_revision_id(str(uuid.uuid4()))
    return policy_builder.get_sav_policy()


def create_fixed_sav_policy(filename):
    sav_policy_builder = _SavPolicyBuilder(FIXED_SAV_POLICY_PATH, filename)
    sav_policy_builder.set_scheduled_scan_day("monday")
    sav_policy_builder.set_scheduled_scan_time("11:00:00")
    sav_policy_builder.set_posix_exclusions(["*.glob", "globExample?.txt", "/stemexample/*"])
    sav_policy_builder.set_sophos_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3"])
    sav_policy_builder.set_user_defined_extension_exclusions(["exclusion1", "exclusion2", "exclusion3", "exclusion4"])
    sav_policy_builder.set_pua_detection("true")
    sav_policy_builder.send_sav_policy()


def give_policy_unique_revision_id(inputPolicyPath, outputFilename):
    sav_policy_builder = _SavPolicyBuilder(inputPolicyPath, outputFilename)
    sav_policy_builder.set_revision_id(str(uuid.uuid4()))
    sav_policy_builder.send_sav_policy()


class _SavPolicyBuilder:
    def __init__(self, input_path, output_name=None):
        self.path = input_path
        if output_name is None:
            self.output_path = None
        else:
            self.output_path = RESOURCES_DIR + "/" + output_name
        # Defaults
        self.replacement_map = {"{{allFiles}}": "false",
                                "{{excludeSophosDefined}}": "",
                                "{{excludeUserDefined}}": "",
                                "{{noExtensions}}": "true",
                                "{{IncludeOnlyTmpExclusions}}":
                                    self._create_tagged_lines(ExclusionHelper.get_exclusions_to_scan_tmp_test(), "filePath")
                                }

    def send_sav_policy(self):
        content = self.get_sav_policy()
        with open(self.output_path, "w") as out:
            out.write(content)

    def get_sav_policy(self):
        return self.get_policy()

    def get_policy(self):
        with open(self.path, "r") as xml:
            policy = xml.read()
        for key, value in self.replacement_map.items():
            policy = policy.replace(key, value)
        return policy

    def add_replacement(self, src, replacement):
        self.replacement_map[src] = replacement

    def set_scheduled_scan_day(self, day):
        self.replacement_map["{{day}}"] = day

    def remove_scheduled_scan_day(self):
        self.replacement_map["<day>{{day}}</day>"] = ""

    def set_scheduled_scan_time(self, time):
        self.replacement_map["{{scheduledScanTime}}"] = time

    def remove_scheduled_scan_time(self):
        self.replacement_map["<time>{{scheduledScanTime}}</time>"] = ""

    def set_posix_exclusions(self, exclusions):
        if isinstance(exclusions, str):
            exclusions = json.loads(exclusions)
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

    def set_on_access(self, value):
        self.replacement_map["{{onAccessEnabled}}"] = 'true' if value else "false"

    def set_on_access_on(self):
        self.replacement_map["{{onAccessEnabled}}"] = 'true'

    def set_boolean(self, value, element):
        value = "true" if value else "false"
        start = f"<{element}>"
        end = f"</{element}>"
        replacement = start + value + end
        self.replacement_map[start + "true" + end] = replacement

    def set_on_access_on_read(self, value, element):
        logger.info(f"On-Read={value}")
        return self.set_boolean(value, element)

    def set_on_access_on_write(self, value, element):
        logger.info(f"On-Write={value}")
        return self.set_boolean(value, element)

    def set_pua_detection(self, pua_detection_enabled):
        self.replacement_map["{{scheduledScanPUAdetection}}"] = pua_detection_enabled

    def set_allowed_pua(self, allowed_pua):
        self.replacement_map["{{puaExclusions}}"] = allowed_pua

    def set_revision_id(self, revision_id):
        self.replacement_map["{{revId}}"] = revision_id


def create_corc_policy(whitelist_sha256s=[],
                       whitelist_paths=[],
                       sxlLookupEnabled=True,
                       sxl_url="https://4.sophosxl.net",
                       revid=None,
                       create_windows_path=False):
    if not revid:
        revid = str(time.time())

    with open(CORC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()

    whitelist_items = []
    for sha256 in whitelist_sha256s:
        whitelist_items.append(f'<item type="sha256">{sha256}</item>')
    for path in whitelist_paths:
        whitelist_items.append(f'<item type="posix-path">{path}</item>')
    if create_windows_path:
        whitelist_items.append(f'<item type="path">\windows\path</item>')

    policy = policy.replace("{{whitelistItems}}", "\n".join(whitelist_items))
    policy = policy.replace("{{sxl4_enable}}", "true" if sxlLookupEnabled else "false")
    policy = policy.replace("{{sxl4_url}}", sxl_url)
    policy = policy.replace("revisionid", revid)
    return policy


def create_sav_policy(revid=None, pua_exclusions="", scheduled_scan=False):
    if not revid:
        revid = str(uuid.uuid4())

    if isinstance(pua_exclusions, str) and not pua_exclusions.startswith("<puaName>"):
        pua_exclusions = pua_exclusions.split(",")

    if isinstance(pua_exclusions, list):
        ex = [ "<puaName>%s</puaName>" % x for x in pua_exclusions ]
        pua_exclusions = "".join(ex)

    scan_template = os.path.join(RESOURCES_DIR, "SAV_Policy_No_Scans.xml")
    if scheduled_scan:
        scan_template = SAV_POLICY_TEMPLATE_PATH
    builder = _SavPolicyBuilder(scan_template)

    builder.set_revision_id(revid)
    builder.set_allowed_pua(pua_exclusions)
    return builder.get_policy()


def populate_alc_policy(revid: str, algorithm: str, username: str, userpass: str,
                        customer_id: str="a1c0f318-e58a-ad6b-f90d-07cabda54b7d"):
    with open(ALC_POLICY_TEMPLATE_PATH) as f:
        policy = f.read()

    policy = policy.replace("{{revid}}", revid)
    policy = policy.replace("{{algorithm}}", algorithm)
    policy = policy.replace("{{username}}", username)
    policy = policy.replace("{{userpass}}", userpass)
    policy = policy.replace("{{customer_id}}", customer_id)
    return policy
