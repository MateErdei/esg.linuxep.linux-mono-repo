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

        return createInstallSet.create_install_set_if_required(install_set, sdds_component, base)

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

    def __set_tap_test_output_permissions(self, dest):
        os.chmod(os.path.join(dest, "SendThreatDetectedEvent"), 0o755)
        os.chmod(os.path.join(dest, "SendDataToSocket"), 0o755)
        os.chmod(os.path.join(dest, "safestore_print_tool"), 0o755)
        os.chmod(os.path.join(dest, "SafeStoreTapTests"), 0o755)
        BuiltIn().set_global_variable("${AV_TEST_TOOLS}", dest)
        return dest

    def unpack_if_newer(self, zippath, dest):
        stat1 = os.stat(zippath)
        try:
            stat2 = os.stat(dest)
            if stat1.st_mtime < stat2.st_mtime:
                return
        except EnvironmentError:
            pass
        logger.info(f"Unpacking {zippath} to {dest}")
        os.makedirs(dest, exist_ok=True)
        with zipfile.ZipFile(zippath) as z:
            for entry in z.infolist():
                logger.info(f"Extracting {entry.filename} from {zippath}")
                z.extract(entry, dest)

    def extract_tap_test_output(self):
        """
        Do more clever extraction based for Bazel builds

        Replaces:
     ${BUILD_ARTEFACTS_FOR_TAP} := ${TEST_INPUT_PATH}/tap_test_output_from_build
    ${result} =   Run Process    tar    xzf    ${BUILD_ARTEFACTS_FOR_TAP}/tap_test_output.tar.gz    -C    ${TEST_INPUT_PATH}
    Log  ${result.stdout}
    Log  ${result.stderr}
    Should Be Equal As Integers   ${result.rc}  ${0}
    Set Global Variable  ${AV_TEST_TOOLS}         ${TEST_INPUT_PATH}/tap_test_output

        :return:
        """
        TEST_INPUT_PATH = BuiltIn().get_variable_value("${TEST_INPUT_PATH}")
        src = os.path.join(TEST_INPUT_PATH, "tap_test_output_from_build", "tap_test_output.tar.gz")
        zip_dbg_src = "/vagrant/esg.linuxep.linux-mono-repo/.output/av/linux_x64_dbg/tap_test_output.zip"
        zip_rel_src = "/vagrant/esg.linuxep.linux-mono-repo/.output/av/linux_x64_rel/tap_test_output.zip"

        dest = TEST_INPUT_PATH + "/tap_test_output"

        if os.path.isfile(zip_dbg_src):
            self.unpack_if_newer(zip_dbg_src, dest)
            logger.info(f"Bazel build - unpacked {zip_dbg_src} in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif os.path.isfile(zip_rel_src):
            self.unpack_if_newer(zip_rel_src, dest)
            logger.info(f"Bazel build - unpacked {zip_rel_src} in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif os.path.isdir(dest):
            # assume already unpacked
            logger.info(f"Bazel build - files in {dest}")
            return self.__set_tap_test_output_permissions(dest)
        elif not os.path.isfile(src):
            logger.error("No src archive or unpacked directory for tap_test_output programs")
            logger.error(f"TEST_INPUT_PATH={TEST_INPUT_PATH}")
            logger.error(f"zip_rel_src={zip_rel_src}")
            logger.error(f"zip_dbg_src={zip_dbg_src}")
            logger.error(f"TEST_INPUT_PATH listing ={os.listdir(TEST_INPUT_PATH)}")
            raise AssertionError("Failed to unpack or find tap_test_output")

        # unpack the tarfile
        result = subprocess.run(["tar", "xzf", src, "-C", TEST_INPUT_PATH], stderr=subprocess.PIPE)
        assert result.returncode == 0, f"Failed to unpack tarfile {src}: ${result.stderr}"
        assert os.path.isdir(dest), f"No correct dest directory created from unpacking tarfile {src}: ${result.stderr}"
        return self.__set_tap_test_output_permissions(dest)
