#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import subprocess

try:
    from . import CentralConnector
    from . import SophosRobotSupport
except ImportError:
    import CentralConnector
    import SophosRobotSupport

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)


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

        SOPHOS_INSTALL = SophosRobotSupport.get_variable("SOPHOS_INSTALL", "/opt/sophos-spl")
        register_central = os.path.join(SOPHOS_INSTALL, "base", "bin", "registerCentral")
        subprocess.check_call([register_central, token, url])

