#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import glob
import json
import os
import shutil
import subprocess

from packaging import version
from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn


def check_files_in_versioned_copy_manifests_have_correct_symlink_versioning():
    check_versioned_copy_file_manifest("changedFiles_manifest.dat", 1)
    check_versioned_copy_file_manifest("addedFiles_manifest.dat", 0)
    

# Find all <x>_manifest.dat files and check the file entries within have correct symlinks
def check_versioned_copy_file_manifest(manifest_name, expected_version):
    expected_version = str(expected_version)
    install_dir = BuiltIn().get_variable_value("${SOPHOS_INSTALL}")
    sophos_tmp_dir = os.path.join(install_dir, "tmp")
    file_manifests = glob.glob("{}/*/{}".format(sophos_tmp_dir, manifest_name))
    if len(file_manifests) == 0:
        raise AssertionError("Could not find any manifests named: {}".format(manifest_name))
    logger.info("{} manifests: {}".format(manifest_name, file_manifests))

    # Example line from a changedFiles_manifest.dat file: files/base/bin/UpdateScheduler
    # which relates to the file /opt/sophos-spl/base/bin/UpdateScheduler

    skip_symlink_check_list = [os.path.join(install_dir, "base/pluginRegistry/mtr.json"),
                               os.path.join(install_dir, "base/pluginRegistry/liveresponse.json"),
                               os.path.join(install_dir, "base/pluginRegistry/eventjournaler.json"),
                               os.path.join(install_dir, "base/pluginRegistry/responseactions.json"),
                               os.path.join(install_dir, "base/pluginRegistry/edr.json")]

    logger.trace("Symlink skip list: {}".format(skip_symlink_check_list))
    
    for manifest_path in file_manifests:
        logger.trace("Manifest path: {}".format(manifest_path))
        with open(manifest_path, "r") as manifest_file:
            for manifest_file_line in manifest_file.readlines():
                manifest_file_line = manifest_file_line.strip()
                logger.trace("{} manifest entry: {}".format(manifest_name, manifest_file_line))

                # Files in root dir do not get installed.
                # plugin registry files do not get version copied
                if "/" not in manifest_file_line or manifest_file_line.startswith("installer/plugins/"):
                    logger.info("Skipping file: {}".format(manifest_file_line))
                    continue

                installed_to_path = os.path.join(install_dir, "/".join(manifest_file_line.strip("/").split("/")[1:]))
               
                if installed_to_path not in skip_symlink_check_list:
                    logger.trace("Expected symlink path: {}".format(installed_to_path))
                    # Expect a symlink similar to: VERSION.ini -> VERSION.ini.3
                    if not os.path.islink(installed_to_path):
                        raise AssertionError("Could not find symlink: {}".format(installed_to_path))
    
                    expected_versioned_file = "{}.{}".format(installed_to_path, expected_version)
                    if not os.path.isfile(expected_versioned_file):
                        raise AssertionError("Could not find file: {}".format(expected_versioned_file))
    
                    logger.trace("Found versioned file: {}".format(expected_versioned_file))
                else:
                    logger.trace("Expected file path: {}".format(installed_to_path))
                    if not os.path.isfile(installed_to_path):
                        raise AssertionError("Could not find file: {}".format(installed_to_path))
  

def strip_all_lines(list_of_strings):
    for idx, value in enumerate(list_of_strings):
        list_of_strings[idx] = list_of_strings[idx].strip()


def convert_to_unix_paths(list_of_paths):
    for idx, value in enumerate(list_of_paths):
        list_of_paths[idx] = list_of_paths[idx].replace("\\", "/").replace("//", "/").replace('"./', '"')
        

def compare_before_and_after_manifests_with_changed_files_manifest(before_manifest_content, after_manifest_content,
                                                                   changed_file_content):
    before_lines = before_manifest_content.split("\n")
    after_lines = after_manifest_content.split("\n")
    changed_lines = changed_file_content.split("\n")

    starts_with_quote = lambda x: x.startswith('"')
    before_lines = list(filter(starts_with_quote, before_lines))
    after_lines = list(filter(starts_with_quote, after_lines))

    strip_all_lines(before_lines)
    strip_all_lines(after_lines)

    convert_to_unix_paths(before_lines)
    convert_to_unix_paths(after_lines)

    difference_in_manifests = list(set(after_lines).difference(set(before_lines)))

    convert_to_unix_paths(difference_in_manifests)

    for idx, a in enumerate(difference_in_manifests):
        difference_in_manifests[idx] = difference_in_manifests[idx].split('" ')[0].strip('"')

    not_empty_string = lambda x: x != ''
    difference_in_manifests = list(filter(not_empty_string, difference_in_manifests))
    changed_lines = list(filter(not_empty_string, changed_lines))

    logger.trace("Changed lines: {}".join(changed_lines))
    logger.trace("Difference in manifests: {}".join(difference_in_manifests))

    if set(difference_in_manifests) != set(changed_lines):
        error_message = """The difference between the before and after upgrade manifests don't match the actual
changed files detected by version copy

manifest diff: 
{}

Version copy changed file: 
{}""".format("\n".join(difference_in_manifests), "\n".join(changed_lines))
        raise AssertionError(error_message)

def check_version_over_1_1_7(version_string):
    logger.info(version_string)
    if version.parse(version_string) >= version.parse("1.1.7"):
        return True
    return False

def dump_plugin_registry_files():
    plugin_registry = "/opt/sophos-spl/base/pluginRegistry"
    for basename in os.listdir(plugin_registry):
        with open(os.path.join(plugin_registry,basename)) as file:
            logger.info(f"{basename} :\n{file.read()}")

def only_subscriptions_in_policy_are_in_alc_status_subscriptions():
    import xml.dom.minidom as xml

    policy_xml_string = open("/opt/sophos-spl/base/mcs/policy/ALC-1_policy.xml").read()
    status_xml_string = open("/opt/sophos-spl/base/mcs/status/ALC_status.xml").read()

    policy_xml = xml.parseString(policy_xml_string)
    status_xml = xml.parseString(status_xml_string)
    logger.info(policy_xml_string)
    logger.info(status_xml_string)

    policy_xml_subscriptions = policy_xml.getElementsByTagName("subscription")
    status_xml_subscriptions = status_xml.getElementsByTagName("subscription")

    status_subscriptions = []
    for subscription in status_xml_subscriptions:
        rigidName = subscription.getAttribute("rigidName")
        assert rigidName != ""
        if rigidName not in status_subscriptions:
            status_subscriptions.append(rigidName)
        else:
            raise AssertionError(f"found duplicate rigidname in status: {rigidName}")

    policy_subscriptions = []
    for subscription in policy_xml_subscriptions:
        rigidName = subscription.getAttribute("RigidName")
        assert rigidName != ""
        if rigidName not in policy_subscriptions:
            policy_subscriptions.append(rigidName)
        else:
            raise AssertionError(f"found duplicate rigidname in policy: {rigidName}")

    policy_subscriptions.sort()
    status_subscriptions.sort()
    logger.info(policy_subscriptions)
    logger.info(status_subscriptions)

    if policy_subscriptions != status_subscriptions:
        raise AssertionError(f"{policy_subscriptions} != {status_subscriptions}")

def copy_files_and_folders_from_within_source_folder(src, dest, recreate_dest=0):
    if recreate_dest == 1 and os.path.exists(dest):
        shutil.rmtree(dest)

    if not os.path.isdir(dest):
        os.makedirs(dest)

    subprocess.call("cp -r {}/* {}".format(src, dest), shell=True)


def check_custom_install_is_set_in_plugin_registry_files(install_path):
    excluded_registry_files = ["sdu.json", "mcsrouter.json", "managementagent.json",
                               "updatescheduler.json", "tscheduler.json"]
    expected_env_vars = {'name': 'SOPHOS_INSTALL', 'value': install_path}

    for (dirpath, dirnames, filenames) in os.walk(f"{install_path}/base/pluginRegistry"):
        for file in filenames:
            if file in excluded_registry_files:
                continue

            with open(os.path.join(dirpath, file)) as registry_file:
                content = json.loads(registry_file.read())

                if expected_env_vars not in content.get("environmentVariables"):
                    raise AssertionError(f"Custom SPL install path is not set correctly in {file} plugin registry file. Contents: {content}")

