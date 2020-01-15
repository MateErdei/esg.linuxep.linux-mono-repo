#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
response_receiver Module
"""

import logging
import os
import re
import json

import mcsrouter.utils.path_manager as path_manager

LOGGER = logging.getLogger(__name__)

def validate_string_as_json(string):
    # Will throw a ValueError if the string is not valid json
    json.loads(string)


def receive():
    """
    This function is to be used as a generator in for loops to yield a tuple
    containing data about LiveQuery response files.
    """

    response_dir = os.path.join(path_manager.response_dir())
    for response_file in os.listdir(response_dir):
        match_object = re.match(r"^([^_]*)_([^_]*)_response\.json$", response_file)
        file_path = os.path.join(response_dir, response_file)
        if match_object:
            app_id = match_object.group(1)
            correlation_id = match_object.group(2)
            time = os.path.getmtime(file_path)
            try:
                with open(file_path, 'r', encoding='utf-8') as file_to_read:
                    body = file_to_read.read()
                    validate_string_as_json(body)
                    yield (file_path, app_id, correlation_id, time, body)
            except json.JSONDecodeError as error:
                LOGGER.error("Failed to load response json file \"{}\". Error: {}".format(file_path, str(error)))
                try:
                    os.remove(file_path)
                except OSError as error:
                    LOGGER.warning("Could not remove response json file \"{}\". Error: {}".format(file_path,
                                                                                                  str(error)))
        else:
            LOGGER.warning("Malformed response file: %s", response_file)
