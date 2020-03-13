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


def exclusions_for_everything_else(inclusion):
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
                LOGGER.info('INCLUDE:', dir_name)
                pass
            else:
                LOGGER.info('exclude', full_path)
                to_remove.append(dir_name)
                exclusions.append(f'<filePath>{full_path}/</filePath>')
        for d in to_remove:
            try:
                dirs.remove(d)
            except ValueError:
                LOGGER.error(f'Failed to remove {d} from {dirs}')
                raise

        for file_name in files:
            full_path = os.path.join(dir_path, file_name)
            if inclusion.startswith(full_path):
                LOGGER.info("INCLUDE:", file_name)
                pass
            else:
                LOGGER.info("exclude:", full_path)
                exclusions.append(f'<filePath>{full_path}</filePath>')

    exclusions.sort()
    return ''.join(exclusions)
