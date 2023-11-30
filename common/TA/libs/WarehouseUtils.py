#!/usr/bin/env python
# Copyright 2019-2023 Sophos Limited. All rights reserved.

import os
import re
import shutil
import xml.etree.ElementTree as ET
import json
import platform
import subprocess

import robot.api.logger as logger
from packaging import version
from robot.libraries.BuiltIn import BuiltIn

from IniUtils import get_version_number_from_ini_file

SDDS3_BUILDER = os.environ.get("SDDS3_BUILDER")


def get_version_from_sdds_import_file(path):
    with open(path) as file:
        contents = file.read()
        xml = ET.fromstring(contents)
        return xml.find(".//Version").text


class WarehouseUtils(object):
    """
    Class to get versions of install components
    """

    ROBOT_LIBRARY_SCOPE = "GLOBAL"
    ROBOT_LISTENER_API_VERSION = 2

    def get_version_for_rigidname_in_sdds3_warehouse(self, warehouse_repo_root, launchdarkly_root, rigidname):
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
        package_name = None
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

            package_name = package_ref.get("src")
            package_path = os.path.join(warehouse_repo_root, "package", package_name)
            temp_dir = "sdds3_temp"
            os.makedirs(temp_dir, exist_ok=True)
            args = [
                SDDS3_BUILDER,
                "--extract-package",
                "--package",
                package_path,
                "--dir",
                temp_dir,
                "--file",
                "VERSION.ini",
            ]
            result = subprocess.run(args, capture_output=True, text=True)
            if result.returncode != 0:
                raise AssertionError(f"Failed to run '{args}': {result.stderr}")
            version = get_version_number_from_ini_file(os.path.join(temp_dir, "VERSION.ini"))
            shutil.rmtree(temp_dir)

            return version

        raise AssertionError(f"Did not find {rigidname} for {arch} in {result.stdout}")

    def second_version_is_lower(self, version1, version2):
        return version.parse(version1) > version.parse(version2)
