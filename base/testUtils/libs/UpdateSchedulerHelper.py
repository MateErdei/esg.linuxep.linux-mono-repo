import os
import subprocess as sp
import sys
import time
import LogUtils
import json
import xml.dom.minidom
import pwd
import grp

from datetime import datetime, timedelta
from robot.libraries.BuiltIn import BuiltIn, RobotNotRunningError
from robot.api import logger

import PathManager


MCS_PATH = "base/mcs"
MCS_STATUS_PATH = os.path.join(MCS_PATH, 'status')
EVENTSPATH = os.path.join(MCS_PATH, 'event')
REPORTSPATH = 'base/update/var'
CONFIGPATH = 'base/update/var/update_config.json'
WDCTL = 'bin/wdctl'
BASE_BIN_PATH = 'base/bin'

RIGIDNAMES_TO_ID = {
    "ServerProtectionLinux-Base": "Base",
    "ServerProtectionLinux-Plugin-MDR": "MDR",
    "ServerProtectionLinux-Plugin-EventProcessor": "SENSORS",
    "ServerProtectionLinux-Plugin-AuditPlugin": "SENSORS"
}

class UpdateSchedulerHelper(object):
    def __init__(self):
        self.suldownloader_config_time = None

    def __del__(self):
        pass

    def get_install(self):
        try:
            return format(BuiltIn().get_variable_value("${SOPHOS_INSTALL}"))
        except RobotNotRunningError:
            try:
                return os.environ.get("SOPHOS_INSTALL", "/opt/sophos-spl")
            except KeyError:
                return "/opt/sophos-spl"

    def __get_reports(self, installdir=None):
        installdir = installdir or self.get_install()
        reportdir = os.path.join(installdir, REPORTSPATH)
        reports = [ entry for entry in os.listdir(reportdir) if entry.startswith('update_report') and entry.endswith('.json')]
        reports.sort()
        return [os.path.join(reportdir, report) for report in reports]

    def disable_mcsrouter(self):
        try:
            print(sp.check_output([os.path.join(self.get_install(), WDCTL), 'stop', 'mcsrouter']))
        except Exception as ex:
            sys.stderr.write(str(ex))

    def replace_sul_downloader_with_fake_broken_version(self):
        broken_sul_downloader_exe = \
        """
        #!/bin/bash   
        exit 1
        """
        open(os.path.join(self.get_install(), BASE_BIN_PATH, "SulDownloader.0"), 'w').write(broken_sul_downloader_exe)

    def check_event_file_generated(self, wait_time_seconds=20, installdir=None):
        """Monitor the event folder for up to 20 seconds to find a ALC_event- file. Flags error if not found."""
        wait_time_seconds_int = int(wait_time_seconds)
        installdir = installdir or self.get_install()
        events_dir = os.path.join(installdir, EVENTSPATH)
        for i in range(wait_time_seconds_int):
            l = os.listdir(events_dir)
            alcEvents = [e for e in l if e.startswith('ALC_event-')]
            if alcEvents:
                return os.path.join(events_dir,  alcEvents[0])
            time.sleep(1)
        raise AssertionError("No Event file found in " + events_dir)

    def check_no_new_event_created(self, previousEventPath, installdir=None):
        """Monitor the event folder for up to 5 seconds to find a ALC_event- file. Flags error if found."""
        eventName = os.path.basename(previousEventPath)
        installdir = installdir or self.get_install()
        events_dir = os.path.join(installdir, EVENTSPATH)
        for i in range(5):
            l = os.listdir(events_dir)
            alcEvents = [e for e in l if e.startswith('ALC_event-') and e != eventName]
            if alcEvents:
                info = "New event created! All events: \n"
                for eventPath in alcEvents:
                    info += "Path: " + eventPath + "\n"
                    info += open(os.path.join(events_dir, eventPath),'r').read()
                    info += "\n"

                raise AssertionError(info)
            time.sleep(1)

    def create_fake_report(self):
        """Create an up-to-date report in the report folder."""
        reportContent=""" 	{
    "finishTime": "20180821 121220",
    "status": "SUCCESS",
    "sulError": "",
    "products": [
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Base",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Base#0.5.0"
        },
        {
            "errorDescription": "",
            "rigidName": "ServerProtectionLinux-Plugin",
            "downloadVersion": "0.5.0",
            "productStatus": "UPTODATE",
            "productName": "ServerProtectionLinux-Plugin#0.5"
        }
    ],
    "startTime": "20180821 121220",
    "errorDescription": "",
    "urlSource": "Sophos",
    "syncTime": "20180821 121220"
}"""
        report_fileName="update_report20180830 163314.json"
        open(os.path.join(self.get_install(), REPORTSPATH, report_fileName), 'w').write(reportContent)

    def get_latest_report_path(self, installdir=None):
        installdir = installdir or self.get_install()
        for i in range(10):
            reports = self.__get_reports(installdir)
            if reports:
                return reports[-1]
            time.sleep(1)
        raise AssertionError("No report found in " + os.path.join(installdir, REPORTSPATH))

    def log_report_files(self, installdir=None):
        installdir = installdir or self.get_install()
        reports = self.__get_reports(installdir)
        if not reports:
            return
        logUtils = LogUtils.LogUtils()
        for file in reports:
            logUtils.dump_log(file)

    def replace_sophos_urls_to_localhost(self, use_update_cache=False, config_path=CONFIGPATH, url='https://localhost:1233'):
        config_path = os.path.join(self.get_install(), config_path)
        logger.info("Editing file {}".format(config_path))
        config = json.load(open(config_path, 'r'))
        logger.info("Original value: {}".format(config))
        config['sophosURLs'] = [url]
        config['certificatePath'] = os.path.abspath(os.path.join(PathManager.get_support_file_path(), 'sophos_certs'))
        config['systemSslPath'] = os.path.abspath(os.path.join(PathManager.get_support_file_path(), 'https/ca'))
        if use_update_cache:
            logger.info("using update cache")
            config['updateCache'] = ['localhost:1236']
            cache_path = os.path.join(PathManager.get_support_file_path(), 'https/ca')
            config['cacheUpdateSslPath'] = os.path.abspath(cache_path)
        filecontent = json.dumps(config, separators=(',', ': '), indent=4)
        logger.info("New content: {}".format(filecontent))
        open(config_path, 'w').write(filecontent)

    def replace_update_cache_url_to_localhost(self):
        config_path = os.path.join(self.get_install(), CONFIGPATH)
        logger.info("Editing file {}".format(config_path))
        config = json.load(open(config_path, 'r'))
        logger.info("Original value: {}".format(config))

        filecontent = json.dumps(config, separators=(',', ': '), indent=4)
        logger.info("New content: {}".format(filecontent))
        open(config_path, 'w').write(filecontent)

    def replace_username_and_password_in_sulconfig(self,username="9539d7d1f36a71bbac1259db9e868231", password="9539d7d1f36a71bbac1259db9e868231", config_path=CONFIGPATH):
        config_path = os.path.join(self.get_install(), config_path)
        logger.info("Editing file {}".format(config_path))
        config = json.load(open(config_path, 'r'))
        logger.info("Original value: {}".format(config))
        config['credential'] = {"username": username, "password": password}
        filecontent = json.dumps(config, separators=(',', ': '), indent=4)
        logger.info("New content: {}".format(filecontent))
        open(config_path, 'w').write(filecontent)


    def replace_credential_for_test(self):
        config_path = os.path.join(self.get_install(), CONFIGPATH)
        logger.info("Editing file {}".format(config_path))
        config = json.load(open(config_path, 'r'))
        logger.info("Original value: {}".format(config))
        config['credential']['username'] = 'regruser'
        config['credential']['password'] = 'regrABC123pass'
        filecontent = json.dumps(config, separators=(',', ': '), indent=4)
        logger.info("New content: {}".format(filecontent))
        open(config_path, 'w').write(filecontent)

    def replace_time_stamp_in_report_file(self, number_of_days_in_past=1):
        report_path = self.get_latest_report()
        logger.info("Editing file {}".format(report_path))
        report = json.load(open(report_path, 'r'))
        logger.info("Original value: {}".format(report))

        timestamp = datetime.now()
        new_timestamp = timestamp.today()-timedelta(days=number_of_days_in_past)

        report['startTime'] = new_timestamp.strftime("%Y%m%d %H%M%S")
        report['finishTime'] = new_timestamp.strftime("%Y%m%d %H%M%S")
        report['syncTime'] = new_timestamp.strftime("%Y%m%d %H%M%S")

        filecontent = json.dumps(report, separators=(',', ': '), indent=4)
        logger.info("New content: {}".format(filecontent))
        open(report_path, 'w').write(filecontent)

    def create_empty_config_file_to_stop_first_update_on_first_policy_received(self):
        config_path = os.path.join(self.get_install(), CONFIGPATH)
        with open(config_path, 'w') as configfile:
            configfile.write('Junk')
        uid = pwd.getpwnam("sophos-spl-user").pw_uid
        gid = grp.getgrnam("sophos-spl-group").gr_gid
        os.chown(config_path, uid, gid)

        if not os.path.exists(config_path):
            raise AssertionError("Failed to generate {}". format(config_path))

    def readIfPresent(self, p):
        try:
            return open(p).read()
        except EnvironmentError:
            return None

    def wait_for_update_status_success(self, timeout):
        statusPath = os.path.join(self.get_install(), MCS_STATUS_PATH, b"ALC_status.xml")
        endTime = time.time() + float(timeout)
        content = b"({} DOES NOT EXIST)".format(statusPath)
        while time.time() < endTime:
            content = self.readIfPresent(statusPath) or content
            if b"<lastResult>0</lastResult>" in content:
                return
            time.sleep(0.5)

        raise AssertionError(b"Status file never contained success: {}: {}".format(statusPath, content))

    def register_current_sul_downloader_config_time(self):
        try:
            config_path = os.path.join(self.get_install(), CONFIGPATH)
            created_time = os.stat(config_path).st_ctime
            self.suldownloader_config_time = created_time
            logger.info("Current Time of suldownloader config: {}".format(self.suldownloader_config_time))
        except OSError:
            # if there is no file available, it is not an error
            self.suldownloader_config_time = None
            pass

    def wait_for_new_sul_downloader_config_file_created(self, timeout = 30):
        future_time = time.time() + timeout
        config_path = os.path.join(self.get_install(), CONFIGPATH)
        while future_time > time.time():
            try:
                created_time = os.stat(config_path).st_ctime
                if self.suldownloader_config_time:
                    if created_time > self.suldownloader_config_time:
                        # file is more recent than last time it was checked.
                        self.suldownloader_config_time = created_time
                        logger.info("Current Time of suldownloader config: {}".format(self.suldownloader_config_time))
                        return
                else:
                    # first time ever the file was downloaded. Update mcs_policy_time and return
                    self.suldownloader_config_time = created_time
                    logger.info("Current Time of suldownloader config: {}".format(self.suldownloader_config_time))
                    return
            except OSError:
                # policy file is not present is not an error. Just means not downloaded yet.
                pass

            time.sleep(1)
        raise AssertionError("Timeout waiting for new config file to be updated")

    def _get_content_of_policy(self, original_file, **kwargs):
        temp_alc_path = os.path.join(self.get_install(), 'tmp/ALC-1_policy.xml')
        if not os.path.exists(original_file):
            search_support = os.path.join(PathManager.get_support_file_path(), 'CentralXml/', original_file)
            if os.path.exists(search_support):
                original_file = search_support
            else:
                raise AssertionError("File not found {}".format(original_file))

        logger.info("Send policy for file: {} ".format(original_file))
        dom = xml.dom.minidom.parseString(open(original_file, 'r').read())
        logger.debug("File loaded")

        features = dom.getElementsByTagName("Features")[0]
        feature_list = kwargs.get("add_features", "").split()

        for feature in feature_list:
            logger.info("Append feature: {}".format(feature))
            feature_xml = dom.createElement('Feature')
            feature_xml.setAttribute("id", feature)
            features.appendChild(feature_xml)

        remove_features_set = set(kwargs.get("remove_features", "").split())
        logger.info(remove_features_set)
        if remove_features_set:
            if features:
                logger.debug('features xml {}'.format(features.toxml()))
                for feature_to_remove in remove_features_set:
                    for feature in features.getElementsByTagName("Feature"):
                        if feature.getAttribute("id") == feature_to_remove:
                            features.removeChild(feature)
                            # this only works if we assume there is only one of the feature to be remove
                            logger.info("removed: {}, from xml".format(feature_to_remove))

        remove_subscriptions_set = set(kwargs.get("remove_subscriptions", "").split())
        if remove_subscriptions_set:
            subscriptions = dom.getElementsByTagName("cloud_subscriptions")
            if subscriptions:
                subscriptions = subscriptions[0]
            logger.debug('subscriptions xml {}'.format(subscriptions.toxml()))
            for subscription in subscriptions.getElementsByTagName('subscription'):
                logger.debug('subscription type={} content={}'.format(type(subscription), subscription.toxml()))
                attribute_id = subscription.getAttribute('Id')
                logger.debug("Check attribute {}".format(attribute_id))
                if attribute_id in remove_subscriptions_set:
                    logger.info("Remove subscription: {}".format(attribute_id))
                    subscriptions.removeChild(subscription)

        add_subscriptions_set = set(kwargs.get("add_subscriptions", "").split())
        if add_subscriptions_set:
            subscriptions = dom.getElementsByTagName("cloud_subscriptions")
            if subscriptions:
                subscriptions = subscriptions[0]
            logger.debug('subscriptions xml {}'.format(subscriptions.toxml()))
            for rigid_name in add_subscriptions_set:
                logger.info("Append subscription: {}".format(rigid_name))
                subscription_xml = dom.createElement('subscription')
                subscription_xml.setAttribute("Id", RIGIDNAMES_TO_ID[rigid_name])
                subscription_xml.setAttribute("RigidName", rigid_name)
                subscription_xml.setAttribute("Tag", "RECOMMENDED")
                subscriptions.appendChild(subscription_xml)

        logger.debug("Create the alc policy")
        logger.debug("Create temp file {}".format(temp_alc_path))
        logger.info(dom.toxml())
        return dom.toxml()

    def simulate_send_policy(self, original_file, **kwargs):
        """Usage: simulate_send_policy( path_to_xml, features to be added). For example:

        simulate_send_policy( ALC_policy_direct.xml, add_features="MDR SENSORS", remove_subscriptions="" )
        remove_subscriptions could be any of: Base, MDR, SENSORS
        """
        temp_alc_path = os.path.join(self.get_install(), 'tmp/ALC-1_policy.xml')
        xml_content = self._get_content_of_policy(original_file, **kwargs)

        alc_path_destination = os.path.join(self.get_install(), 'base/mcs/policy/ALC-1_policy.xml')

        logger.debug("Create the alc policy")
        logger.debug("Create temp file {}".format(temp_alc_path))
        with open(temp_alc_path, 'w') as f:
            f.write(xml_content)
        logger.debug("Move file to {}".format(alc_path_destination))
        os.rename(temp_alc_path, alc_path_destination)

    def check_report_for_missing_package(self, error_description):
        report = self.get_latest_report_dict()
        missing_status = "PACKAGESOURCEMISSING"

        if "status" not in report:
            raise AssertionError("The status key was not present in the report json")

        if "errorDescription" not in report:
            raise AssertionError("The errorDescription key was not present in the report json")

        if report["status"] != missing_status:
            raise AssertionError("Status was not: {} but was: {}".format(missing_status, report["status"]))

        if report["errorDescription"] != error_description:
            raise AssertionError("errorDescription was not: {} but was: {}".format(error_description, report["errorDescription"]))

    def get_latest_report_dict(self):
        newest = self.get_latest_report_path()
        with open(newest, "r") as report:
            return json.load(report)

    def replace_version(self,old_version, new_version, base_dist):
        sdds_import_path = os.path.join(base_dist, "SDDS-Import.xml")
        version_file_path = os.path.join(base_dist, "files/base/VERSION.ini")
        if not os.path.isfile(version_file_path):
            raise AssertionError("Version file not found {}".format(version_file_path))
        if not os.path.isfile(sdds_import_path):
            raise AssertionError("Version file not found {}".format(sdds_import_path))


        with open(version_file_path, "rt") as f:
            content = f.read()
        content = content.replace(old_version,new_version)
        with open(version_file_path, "wt") as f:
                f.write(content)

        with open(sdds_import_path, "rt") as f:
            content = f.read()
        content = content.replace(old_version,new_version)
        with open(sdds_import_path, "wt") as f:
            f.write(content)
