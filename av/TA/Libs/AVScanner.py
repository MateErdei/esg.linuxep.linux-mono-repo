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
    exclusions = [ '<filePath>{}<filePath>'.format(f) for f in exclusions ]
    return ''.join(exclusions)
