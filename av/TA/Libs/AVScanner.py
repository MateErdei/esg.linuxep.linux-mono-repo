#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import sys
import os
import logging

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


def increase_threat_detector_log_to_max_size_by_path(log_path):
    """
    Increase log_path to maxFileSize-1
    maxFileSize comes from FileLoggingSetup.cpp from SSPL-Base

    const long maxFileSize = 10 * 1024 * 1024;

    :param log_path:
    :return:
    """
    max_size = 10 * 1024 * 1024 - 1
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
    extra += "\n" * last_additional_required
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


def find_score(word):

    with open("/opt/sophos-spl/plugins/av/log/sophos_threat_detector/sophos_threat_detector.log", "r") as fr:
        for line in fr:
            if word in line:
                score = line.split(" ")

    if len(score) == 0:
        return 0

    return score[len(score) - 1]


def check_initial_scores(primary, secondary):
    # change 15 to 20 when scores are corrected
    if int(primary) > 30 and int(secondary) > 15:
        return 1
    return 0


def check_second_score(primary):
    if int(primary) < 30:
        return 1
    return 0
