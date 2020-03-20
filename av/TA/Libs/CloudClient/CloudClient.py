#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

try:
    from . import CentralConnector
except ImportError:
    import CentralConnector

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
