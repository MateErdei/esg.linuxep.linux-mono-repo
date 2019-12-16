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
    Async receive call
    """

    response_dir = os.path.join(path_manager.response_dir())
    for response_file in os.listdir(response_dir):
        match_object = re.match(r"([A-Z]*)_([A-Za-z0-9]*)_response.json", response_file)
        file_path = os.path.join(response_dir, response_file)
        if match_object:
            app_id = match_object.group(1)
            correlation_id = match_object.group(2)
            time = os.path.getmtime(file_path)
            try:
                with open(file_path, 'r', encoding='utf-8') as file_to_read:
                    body = file_to_read.read()
                    validate_string_as_json(body)
                    yield (app_id, correlation_id, time, body)
            except json.JSONDecodeError as error:
                LOGGER.error("Failed to load response json file \"{}\". Error: {}".format(file_path, str(error)))
                continue
        else:
            LOGGER.warning("Malformed response file: %s", response_file)

        os.remove(file_path)
