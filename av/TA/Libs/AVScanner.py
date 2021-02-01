#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import sys
import os
import logging
import tarfile
import zipfile

LOGGER = logging.getLogger(__name__)


def create_long_path(dirname, depth, root='/', file="file", file_contents=""):
    directory_to_return_process_to = os.getcwd()
    os.chdir(root)

    try:
        for i in range(depth):
            if not os.path.exists(dirname):
                os.mkdir(dirname)
            os.chdir(dirname)
    except Exception as e:
        sys.exit(2)

    with open(file, 'w') as file:
        file.write(file_contents)
        file_path = file.name

    directory_to_return = os.getcwd()
    os.chdir(directory_to_return_process_to)

    return directory_to_return


def get_exclusion_list_for_everything_else(inclusion):
    inclusion = inclusion.rstrip('/')
    exclusions = []
    for dir_path, dirs, files in os.walk('/'):
        if dir_path.startswith(inclusion):
            dirs[:] = []
            continue

        to_remove = []
        for dir_name in dirs:
            full_path = os.path.join(dir_path, dir_name)
            if inclusion.startswith(full_path):
                # We want to exclude things in this directory, but not this directory
                LOGGER.debug('INCLUDE: %s', dir_name)
                pass
            else:
                LOGGER.debug('exclude %s', full_path)
                to_remove.append(dir_name)
                exclusions.append(f'{full_path}/')
        for d in to_remove:
            try:
                dirs.remove(d)
            except ValueError:
                LOGGER.error(f'Failed to remove {d} from {dirs}')
                raise

        for file_name in files:
            full_path = os.path.join(dir_path, file_name)
            if inclusion.startswith(full_path):
                LOGGER.debug("INCLUDE: %s", full_path)
                pass
            else:
                LOGGER.debug("exclude: %s", full_path)
                exclusions.append(full_path)

    exclusions.sort()
    return exclusions

def exclusions_for_everything_else(inclusion):
    exclusions = get_exclusion_list_for_everything_else(inclusion)
    exclusions = ['<filePath>{}</filePath>'.format(f) for f in exclusions]
    return ''.join(exclusions)


def increase_threat_detector_log_to_max_size_by_path(log_path, remaining=1):
    """
    Increase log_path to maxFileSize - remaining
    maxFileSize comes from FileLoggingSetup.cpp from SSPL-Base

    const long maxFileSize = 10 * 1024 * 1024;

    :param log_path: location of the log to fill
    :param remaining: (optional) number of bytes to leave, defaults to 1
    :return:
    """
    max_size = 10*1024*1024 - remaining
    statbuf = os.stat(log_path)
    current_size = statbuf.st_size
    additional_required = max_size - current_size

    if additional_required <= 0:
        return

    extra_text = "THIS IS TEST EXTRA TEXT: 01234567890123456789\n"
    copies = additional_required // len(extra_text)
    extra = extra_text * copies

    last_additional_required = additional_required - len(extra)
    assert last_additional_required >= 0
    if last_additional_required > 0:
        extra += "." * (last_additional_required - 1) + "\n"
    assert len(extra) == additional_required

    open(log_path, "ab").write(extra.encode("UTF-8"))
    statbuf = os.stat(log_path)
    current_size = statbuf.st_size
    assert current_size == max_size


def count_eicars_in_directory(d):
    """
    Count files in directory; assume all are eicar
    :param d:
    :return:
    """
    count = 0
    for _, _, files in os.walk(d):
        count += len(files)
    return count


def find_score(word, file_contents):
    score = []
    split_file = file_contents.split('\n')
    for line in split_file:
        if word in line:
            score = line.split(" ")

    if len(score) == 0:
        return 0

    return score[len(score) - 1]


def check_ml_scores_are_above_threshold(actual_primary, actual_secondary, threshold_primary, threshold_secondary):
    if int(actual_primary) > threshold_primary and int(actual_secondary) > threshold_secondary:
        return
    raise AssertionError("ML scores are not above threshold: Primary {} < {} or Secondary {} < {}".format(
        actual_primary,
        threshold_primary,
        actual_secondary,
        threshold_secondary
    ))


def check_ml_primary_score_is_below_threshold(actual_primary, threshold_primary):
    return int(actual_primary) < threshold_primary


def create_tar(path, file, tar_name):
    #not moving the cwd was archiving the full folder tree to the file we wanted to archive
    starting_wd = os.getcwd()
    os.chdir(path)
    with tarfile.open(tar_name, "w") as tar_archive:
        tar_archive.add(file)
    os.chdir(starting_wd)


def create_zip(path, file, zip_name):
    #not moving the cwd was archiving the full folder tree to the file we wanted to archive
    starting_wd = os.getcwd()
    os.chdir(path)
    with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zip_archive:
        zip_archive.write(file)
    os.chdir(starting_wd)


def create_archive_test_files(path):
    create_tar(path, "eicar", "eicar1.tar")
    for x in range(2,17):
        file_to_tar = "eicar" + str(x - 1) + ".tar"
        tar_name = "eicar" + str(x) + ".tar"

        create_tar(path, file_to_tar, tar_name)

    create_zip(path, "eicar", "eicar1.zip")
    for x in range(2,31):
        file_to_tar = "eicar" + str(x - 1) + ".zip"
        tar_name = "eicar" + str(x) + ".zip"

        create_zip(path, file_to_tar, tar_name)


def restore_etc_hosts(path="/etc/hosts"):
    backup = path+"_avscanner_backup"
    if os.path.isfile(backup):
        os.unlink(path)
        os.rename(backup, path)


def alter_etc_hosts(path="/etc/hosts", newline="127.0.0.1 really_fake_server"):
    backup = path+"_avscanner_backup"
    if os.path.isfile(backup):
        restore_etc_hosts()

    contents = open(path).read()
    open(backup, "w").write(contents)
    contents += "\n"+newline+"\n"
    open(path, "w").write(contents)
