#!/usr/bin/env python
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.

import glob
import os
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
                               os.path.join(install_dir, "base/pluginRegistry/edr.json")]

    "base/pluginRegistry/liveresponse.json"
    logger.trace("Symlink skip list: {}".format(skip_symlink_check_list))
    
    for manifest_path in file_manifests:
        logger.trace("Manifest path: {}".format(manifest_path))
        with open(manifest_path, "r") as manifest_file:
            for manifest_file_line in manifest_file.readlines():
                manifest_file_line = manifest_file_line.strip()
                logger.trace("{} manifest entry: {}".format(manifest_name, manifest_file_line))

                # Files in root dir do not get installed.
                if "/" not in manifest_file_line:
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
