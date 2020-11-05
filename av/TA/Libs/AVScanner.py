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


def find_score(word, file_contents):
    score = []
    for line in file_contents:
        if word in line:
            score = line.split(" ")

    if len(score) == 0:
        return 0

    return score[len(score) - 1]


def check_ml_scores_are_above_threshold(actual_primary, actual_secondary, threshold_primary, threshold_secondary):
    # change 15 to 20 when scores are corrected
    return int(actual_primary) > int(threshold_primary) and int(actual_secondary) > int(threshold_secondary)


def check_ml_primary_score_is_below_threshold(actual_primary, threshold_primary):
    return int(actual_primary) < int(threshold_primary)
