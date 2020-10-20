#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import os
import shutil
import subprocess
import time

try:
    from . import CentralConnector
    from . import SophosRobotSupport
    from . import NextScanTime
except ImportError:
    import CentralConnector
    import SophosRobotSupport
    import NextScanTime

try:
    from .. import PathManager
    from .. import AVScanner
except ImportError:
    from Libs import PathManager
    from Libs import AVScanner


try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)


DEV = CentralConnector.DEV  # "DEV"
QA = CentralConnector.QA    # "QA"
PROD = "PROD"

class CloudClient(object):
    def __init__(self):
        self.__m_region = None
        self.__m_connector = None

    def __connector(self):
        assert self.__m_region is not None
        if self.__m_connector is None:
            self.__m_connector = CentralConnector.CentralConnector(self.__m_region)
        return self.__m_connector

    def select_central_region(self, region="AUTO"):
        if region == "AUTO":
            region = os.environ.get("MCS_REGION", "QA")
        self.__m_region = region
        version = self.get_central_version()
        logger.info(f"Connecting to Central Region {region} version {version}")
        self.__connector().setFlagsOnce('server.sspl.antivirus.enabled')

    def get_central_version(self):
        v = self.__connector().getCloudVersion()
        return v

    def __install_certificate_for_region(self, SOPHOS_INSTALL):
        resources = PathManager.get_resources_path()
        certs_dir = os.path.join(resources, "certs")
        cert = None
        if self.__m_region == DEV:
            cert = os.path.join(certs_dir, "hmr-dev-sha1.pem")
        elif self.__m_region == QA:
            cert = os.path.join(certs_dir, "hmr-qa-sha1.pem")
        else:
            logger.debug("No override certificate required/available")

        if cert is not None:
            OVERRIDE_FLAG_FILE = os.path.join(SOPHOS_INSTALL, "base/mcs/certs/ca_env_override_flag")
            open(OVERRIDE_FLAG_FILE, "w").close()
            dest = os.path.join(SOPHOS_INSTALL, "test-mcs-ca.pem")
            shutil.copy(cert, dest)
            os.environ['MCS_CA'] = dest

    def register_in_central(self, token=None, url=None):
        """
        exec ${SOPHOS_INSTALL}/base/bin/registerCentral "${MCS_TOKEN}" "${MCS_URL}"
        :param token: None means get from Central
        :param url:
        :return:
        """
        if url is None and token is not None:
            raise Exception("Inconsistent arguments, token not None and url is None")

        if token is None:
            token, url = self.__m_connector.get_token_and_url_for_registration()

        assert token is not None
        assert url is not None

        SOPHOS_INSTALL = SophosRobotSupport.get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
        assert SOPHOS_INSTALL is not None
        register_central = os.path.join(SOPHOS_INSTALL, "base", "bin", "registerCentral")

        self.__install_certificate_for_region(SOPHOS_INSTALL)

        # subprocess.run added in python 3.5, rework if we need to support an older version
        result = subprocess.run([register_central, token, url], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if result.returncode != 0:
            logger.error("register_central failed: return={}".format(result.returncode))
            logger.error("output: {}".format(result.stdout))
        result.check_returncode()  # throws if the command failed

    def configure_exclude_everything_else_in_central(self, includeDirectory):
        exclusions = AVScanner.get_exclusion_list_for_everything_else(includeDirectory)
        assert isinstance(exclusions, list)
        return self.__m_connector.configure_exclusions(exclusions)

    def ensure_av_policy_exists(self):
        return self.__m_connector.ensure_av_policy_exists()

    def wait_for_computer_to_appear_in_central(self):
        return self.__m_connector.waitForServerInCloud()

    def assign_antivirus_product_to_endpoint_in_central(self):
        return self.__m_connector.assign_antivirus_product_to_endpoint_in_central()

    def send_scan_now_in_central(self):
        return self.__m_connector.scanNow()

    def log_central_events(self):
        events = self.__m_connector.getEvents("ScanComplete", time.time() - 3600)
        logger.debug(repr(events))

    def wait_for_scan_completion_in_central(self, start_time):
        # Request URL: https://dzr-api-amzn-eu-west-1-f9b7.api-upe.d.hmr.sophos.com/api/events?
        #           endpoint=d2bed399-e0b8-0482-1b93-bf361d7ac2ae&
        #           from=2020-03-24T00:00:00.000Z&
        #           limit=50&offset=0&to=2020-03-24T11:24:36.505Z
        assert start_time <= time.time(), f"start_time {start_time} is after now"
        timeout = 30
        timelimit = time.time() + timeout
        events = None
        while time.time() < timelimit:
            events = self.__m_connector.getEvents("ScanComplete", start_time)
            items = events['items']
            if len(items) > 0:
                logger.debug("Got scan completion")
                return True
            time.sleep(5)
        logger.error("Failed to fond scan completion: Available events: "+repr(events))
        raise Exception("Failed to detect scan completion")

    def wait_for_scheduled_scan_completion_in_central(self, start_time):
        timeout = 30
        timelimit = time.time() + timeout
        events = None
        while time.time() < timelimit:
            events = self.__m_connector.getEvents("ScanComplete", start_time)
            items = events['items']
            if len(items) > 0:
                logger.debug("Got scan completion: "+repr(items))
                return True
            time.sleep(5)
        logger.info("Available events: "+repr(events))
        raise Exception("Failed to detect scan completion")

    def clear_alerts_in_central(self):
        self.__m_connector.clearAllAlerts()

    def __check_eicar_reported_in_central(self, location, start_time):
        threatName = "EICAR-AV-Test"
        start_time_str = time.strftime("%Y-%m-%dT%H:%M:%S.000Z", time.gmtime(start_time))
        expected_description = f"Manual cleanup required: '{threatName}' at '{location}'"
        for event in self.__m_connector.generateAlerts():
            logger.info(repr(event))
            when = event['when']
            if when < start_time_str:
                logger.debug("Too early")
                continue

            # "Manual cleanup required: 'EICAR' at '/tmp/testeicar/eicar.com'"
            actual_description = event['description']
            if actual_description != expected_description:
                logger.debug("Wrong description: expected: {}, actual: {}".format(expected_description, actual_description))
                continue

            return True

        return False

    def wait_for_eicar_detection_in_central(self, location, start_time, timeout=30):
        timelimit = time.time() + timeout
        events = None
        while time.time() < timelimit:
            if self.__check_eicar_reported_in_central(location, start_time):
                return True
            time.sleep(5)
        logger.info("Available events: "+repr(events))
        raise Exception("Failed to detect eicar report")

    def configure_next_available_scheduled_scan_in_central(self, includeDirectory):
        scan_time = NextScanTime.get_next_scan_time()
        logger.info("Scheduling scan for {}".format(scan_time.strftime("%H:%M")))
        exclusions = AVScanner.get_exclusion_list_for_everything_else(includeDirectory)
        assert isinstance(exclusions, list)
        self.__m_connector.configure_scheduled_scan_time(scan_time, exclusions)
        return scan_time.strftime("%H:%M")

    @staticmethod
    def evaluate_time_sources(utc_epoch, local_epoch, utc_formatted, local_formatted):
        """
        Investigating "Get Current Date" UTC result_format=epoch
        Found https://github.com/robotframework/robotframework/issues/3306
        DateTime: `Get Current Date` with epoch format and timezone UTC return wrong value
        :param utc_epoch:
        :param local_epoch:
        :param utc_formatted:
        :param local_formatted:
        :return:
        """
        now = time.time()
        logger.info(f"Robot get current date  UTC    epoch = {utc_epoch}")
        logger.info(f"Robot get current date  local  epoch = {local_epoch}")
        logger.info(f"Python                         epoch = {now}")

        logger.info(f"Robot  UTC   Robot             formatted: {utc_formatted}")
        logger.info(f"Python UTC  python gmtime      formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.gmtime(now)))
        logger.info(f"Robot  UTC  python gmtime      formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.gmtime(utc_epoch)))
        logger.info(f"Robot  UTC  python localtime   formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.localtime(utc_epoch)))
        logger.info(f"Robot  local                   formatted: {local_formatted}")
        logger.info(f"Python local                   formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.localtime(now)))
        logger.info(f"Robot  local  python gmtime    formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.gmtime(local_epoch)))
        logger.info(f"Robot  local  python localtime formatted: "+time.strftime("%Y-%m-%d %H:%M:%S.%f", time.localtime(local_epoch)))

        assert (now - utc_epoch) < 1

