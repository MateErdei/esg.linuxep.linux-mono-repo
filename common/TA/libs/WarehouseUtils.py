#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import datetime
import hashlib
import os
import re
import shutil
import subprocess
import xml.etree.ElementTree as ET
import json
import platform

import requests
import robot.api.logger as logger
from packaging import version
from robot.libraries.BuiltIn import BuiltIn

import PathManager
import UpdateServer

THIS_FILE = os.path.realpath(__file__)
LIBS_DIR = PathManager.get_libs_path()
PRODUCT_TEST_ROOT_DIRECTORY = PathManager.get_testUtils_dir()

SUPPORT_FILE_PATH = PathManager.get_support_file_path()


SYSTEMPRODUCT_TEST_INPUT = os.environ.get("SYSTEMPRODUCT_TEST_INPUT", default="/tmp/system-product-test-inputs")
SDDS3_BUILDER = os.environ.get("SDDS3_BUILDER")

ENV_KEY = "env_key"

BUILD_TYPE = "build_type"
PROD_BUILD_CERTS = "prod"
DEV_BUILD_CERTS = "dev"

DEV_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "rootca.crt")
DEV_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "ps_rootca.crt")
PROD_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "rootca.crt")
PROD_PS_ROOT_CA = os.path.join(SUPPORT_FILE_PATH, "sophos_certs", "prod_certs", "ps_rootca.crt")

ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION = os.path.join("base", "update", "rootcerts", "ps_rootca.crt")
ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "rootca.crt")
PS_ROOT_CA_INSTALLATION_EXTENSION_OLD = os.path.join("base", "update", "certs", "ps_rootca.crt")


def _get_sophos_install_path():
    sophos_install_path = BuiltIn().get_variable_value("${SOPHOS_INSTALL}", "/opt/sophos-spl")
    if not os.path.isdir(sophos_install_path):
        raise OSError(f"sophos install path: \"{sophos_install_path}\", does not exist")
    return sophos_install_path


def _install_upgrade_certs(root_ca, ps_root_ca):
    sophos_install_path = _get_sophos_install_path()
    logger.info(f"root_ca:  {root_ca}")
    logger.info(f"ps_root_ca:  {ps_root_ca}")

    try:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION))
    except:
        shutil.copy(root_ca, os.path.join(sophos_install_path, ROOT_CA_INSTALLATION_EXTENSION_OLD))
        shutil.copy(ps_root_ca, os.path.join(sophos_install_path, PS_ROOT_CA_INSTALLATION_EXTENSION_OLD))


def get_version_from_sdds_import_file(path):
    with open(path) as file:
        contents = file.read()
        xml = ET.fromstring(contents)
        return xml.find(".//Version").text

class WarehouseUtils(object):
    """
    Class to setup ALC Policy files used in tests.
    """

    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    ROBOT_LISTENER_API_VERSION = 2

    update_server = UpdateServer.UpdateServer("ostia_update_server.log")



    def generate_local_ssl_certs_if_they_dont_exist(self):
        server_https_cert = os.path.join(SUPPORT_FILE_PATH, "https", "ca", "root-ca.crt.pem")
        if not os.path.isfile(server_https_cert):
            self.update_server.generate_update_certs()

    # TODO LINUXDAR-8265: Remove is_using_version_workaround
    def get_version_for_rigidname_in_sdds3_warehouse(self, warehouse_repo_root, launchdarkly_root, rigidname, is_using_version_workaround=False):
        # Open LaunchDarkly file to get SUS response for the suite
        with open(os.path.join(launchdarkly_root, "release.linuxep.ServerProtectionLinux-Base.json")) as f:
            launchdarkly = json.loads(f.read())

        suite = launchdarkly["RECOMMENDED"]["suite"]
        suite_path = os.path.join(warehouse_repo_root, "suite", suite)
        subprocess.run(["chmod", "+x", SDDS3_BUILDER], check=True)
        args = [SDDS3_BUILDER, "--unpack-signed-file", suite_path]
        result = subprocess.run(args, capture_output=True, text=True)
        if result.returncode != 0:
            raise AssertionError(f"Failed to run '{args}': {result.stderr}")

        arch = platform.machine()

        suite_data = ET.fromstring(result.stdout)
        for package_ref in suite_data.iter("package-ref"):
            line_id = package_ref.find("line-id").text
            logger.debug(f"Checking package: {line_id}")
            if line_id != rigidname:
                logger.debug(f"rigidname differs: {rigidname} != {line_id}")
                continue
            has_matching_platform = False
            for platform_elem in package_ref.find("platforms").iter("platform"):
                platform_name = platform_elem.get("name")
                if arch == "x86_64" and platform_name == "LINUX_INTEL_LIBC6":
                    has_matching_platform = True
                    break
                if arch == "aarch64" and platform_name == "LINUX_ARM64":
                    has_matching_platform = True
                    break
            if not has_matching_platform:
                logger.debug("Doesn't have platform matching current architecture")
                continue

            version = package_ref.find("version").text
            logger.debug(f"Found matching product with version: {version}")
            if is_using_version_workaround:
                return ".".join(version.split(".")[:3] + version.split(".")[4:])
            else:
                return version

        raise AssertionError(f"Did not find {rigidname} for {arch} in {result.stdout}")


    def second_version_is_lower(self, version1, version2):
        return version.parse(version1) > version.parse(version2)


# If ran directly, file sets up local warehouse directory from filer6
if __name__ == "__main__":
    pass
