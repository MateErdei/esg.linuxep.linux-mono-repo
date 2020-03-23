#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import shutil
import subprocess

try:
    from . import CentralConnector
    from . import SophosRobotSupport
except ImportError:
    import CentralConnector
    import SophosRobotSupport

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

    def check_machine_present_in_central(self):
        raise AssertionError("Not implemented")

    def send_scan_now_in_central(self):
        raise AssertionError("Not implemented")

    def clear_alerts_in_central(self):
        raise AssertionError("Not implemented")

    def check_eicar_reported_in_central(self):
        raise AssertionError("Not implemented")

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
