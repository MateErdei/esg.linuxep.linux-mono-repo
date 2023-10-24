#!/usr/bin/env python3
# Copyright 2023 Sophos Limited. All rights reserved.

"""
filesystem_utils Module
"""

import codecs
import logging
import os

LOGGER = logging.getLogger(__name__)


def utf8_write(path, data):
    """
    utf8_write
    :param path:
    :param data:
    """
    try:
        if not isinstance(data, str):
            data = str(data, 'utf-8', 'replace')
        # Between Python 3.7 and 3.8, codecs.py changed their buffering default from 1 to -1. Now we explicitly set here.
        with codecs.open(path, mode="w", encoding='utf-8', buffering=-1) as file_to_write:
            file_to_write.write(data)
    except (OSError, IOError) as exception:
        LOGGER.warning("utf8 write failed with message: %s", str(exception))

def atomic_write(path, tmp_path, mode, data):
    """
    atomic_write
    :param path:
    :param tmp_path:
    :param mode:
    :param data:
    """
    try:
        utf8_write(tmp_path, data)
        os.chmod(tmp_path, mode)
        os.rename(tmp_path, path)
    except (OSError, IOError) as exception:
        LOGGER.warning(f"Atomic write failed with message: {str(exception)}")


def read_file_if_exists(file_path):
    try:
        if not os.path.exists(file_path):
            return None
        with open(file_path) as file_to_read:
            contents = file_to_read.read()
        if contents:
            return contents
    except PermissionError as exception:
        LOGGER.warning(f"Could not access {file_path} with error : {exception}")
        return None
    except OSError as exception:
        LOGGER.warning(f"Could not read contents of {file_path} with error : {exception}")
        return None


# beginning_of_lines_to_find is a list of strings
def return_lines_from_file(file_path, beginning_of_lines_to_find):
    try:
        if not os.path.exists(file_path):
            return None

        matched_lines = {}
        with open(file_path) as check_file:
            for line in check_file.readlines():
                line = line.strip()

                for beginning_of_line_to_find in beginning_of_lines_to_find:
                    if line.startswith(beginning_of_line_to_find):
                        matched_lines.setdefault(beginning_of_line_to_find, []).append(line)
                        break

            return matched_lines
    except PermissionError as exception:
        LOGGER.warning(f"Could not access {file_path} with error : {exception}")
    except OSError as exception:
        LOGGER.warning(f"Could not read contents of {file_path} with error : {exception}")

    return None


def return_line_from_file(file_path, beginning_of_line_to_find):
    try:
        if not os.path.exists(file_path):
            return None
        with open(file_path) as check_file:
            for line in check_file.readlines():
                line = line.strip()
                if line.startswith(beginning_of_line_to_find):
                    return line
    except PermissionError as exception:
        LOGGER.warning(f"Could not access {file_path} with error : {exception}")
    except OSError as exception:
        LOGGER.warning(f"Could not read contents of {file_path} with error : {exception}")
    return None

