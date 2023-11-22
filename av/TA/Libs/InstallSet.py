#!/usr/bin/env python
# Copyright 2020-2023 Sophos Limited. All rights reserved.

import os
import shutil
import sys
import subprocess
import zipfile

try:
    from robot.api import logger
    logger.warning = logger.warn
except ImportError:
    import logging
    logger = logging.getLogger(__name__)

from robot.libraries.BuiltIn import BuiltIn

Libs = os.path.dirname(__file__)
TA = os.path.dirname(Libs)
manual = os.path.join(TA, "manual")
assert os.path.isdir(manual)
if manual not in sys.path:
    sys.path.append(manual)

import createInstallSet
createInstallSet.LOGGER = logger

import downloadSupplements
downloadSupplements.LOGGER = logger
ensure_binary = downloadSupplements.ensure_binary


class InstallSet(object):
    def __init__(self):
        self.__m_install_set_verified = False

    def verify_install_set(self, install_set, sdds_component=None):
        return createInstallSet.verify_install_set(install_set, sdds_component)

    def create_install_set(self, install_set):
        base = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        logger.info("BASE = %s" % base)
        base = ensure_binary(base)
        logger.info("install_set = %s" % install_set)
        install_set = ensure_binary(install_set)
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        logger.info("sdds_component = %s" % sdds_component)
        sdds_component = ensure_binary(sdds_component)

        if not os.path.isdir(sdds_component):
            logger.error("Failed to find SDDS_COMPONENT for INSTALL_SET: %s" % sdds_component)
            return

        logger.info("Creating install set")
        return createInstallSet.create_install_set(install_set, sdds_component, base)

    def Create_Install_Set_If_Required(self):
        if self.__m_install_set_verified:
            return

        # Read variables
        install_set = BuiltIn().get_variable_value("${COMPONENT_INSTALL_SET}")
        sdds_component = BuiltIn().get_variable_value("${COMPONENT_SDDS_COMPONENT}")
        self.__m_install_set_verified = self.verify_install_set(install_set, sdds_component)
        if self.__m_install_set_verified:
            return
        self.create_install_set(install_set)
