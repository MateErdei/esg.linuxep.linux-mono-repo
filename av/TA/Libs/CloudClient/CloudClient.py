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


import logging
logger = logging.getLogger(__name__)


DEV = "DEV"
QA = "QA"
PROD = "PROD"

class CloudClient(object):
    def __init__(self):
        self.__m_region = None
        self.__m_connector = None
        self.select_central_region("DEV")

    def select_central_region(self, region="DEV"):
        self.__m_region = region
        self.__m_connector = CentralConnector.CentralConnector(region)
        self.__m_connector.setFlagsOnce('server.sspl.antivirus.enabled')

    def get_central_version(self):
        v = self.__m_connector.getCloudVersion()
        logger.info("Central Version {}".format(v))
        return v

    def __install_certificate_for_region(self, SOPHOS_INSTALL):
        OVERRIDE_FLAG_FILE = os.path.join(SOPHOS_INSTALL, "base/mcs/certs/ca_env_override_flag")
        open(OVERRIDE_FLAG_FILE, "w").close()
        resources = PathManager.get_resources_path()
        cert = None
        if self.__m_region == DEV:
            cert = os.path.join(resources, "certs/hmr-dev-sha1.pem")

        if cert is not None:
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
        logger.info("Available events: "+repr(events))
        raise Exception("Failed to detect scan completion")

    def clear_alerts_in_central(self):
        self.__m_connector.clearAllAlerts()

    def __check_eicar_reported_in_central(self, location, start_time):
        threatName = "EICAR"
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
                logger.debug("Wrong description: "+actual_description)
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

    @staticmethod
    def _get_next_scan_time(self):
        now = datetime.datetime.now()
        if now.minute < 30:
            now = now.replace(minute=30, second=0, microsecond=0)
        else:
            now = now.replace(minute=0, second=0, microsecond=0)
            now += datetime.timedelta(hours=1)
        return now

    def configure_next_available_scheduled_scan_in_central(self):
        scan_time = self._get_next_scan_time()
        logger.info("Scheduling scan for {}".format(scan_time.isoformat()))
        raise Exception("Configure next available scheduled Scan in Central not implemented")
