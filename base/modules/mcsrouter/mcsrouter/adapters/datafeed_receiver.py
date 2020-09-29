#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

"""
datafeed_receiver Module
"""

import logging
import os
import re
import json

import mcsrouter.utils.path_manager as path_manager

LOGGER = logging.getLogger(__name__)

def validate_string_as_json(string):
    # Will throw a ValueError if the string is not valid json (this includes empty string)
    json.loads(string)

def remove_datafeed_file(file_path):
    if os.path.isfile(file_path):
        try:
            os.remove(file_path)
        except OSError as error:
            LOGGER.warning("Could not remove datafeed json file \"{}\". Error: {}".format(file_path, str(error)))

def pending_files():
    datafeed_dir = os.path.join(path_manager.datafeed_dir())
    number_of_files = len(os.listdir(datafeed_dir))
    return number_of_files > 0

def receive():
    """
    This function is to be used as a generator in for loops to yield a tuple
    containing data about DataFeed files.
    """

    datafeed_dir = os.path.join(path_manager.datafeed_dir())
    for datafeed_file in os.listdir(datafeed_dir):
        # <datafeedid>-<timestamp>, e.g. scheduled_query-1234567
        # Unfortunately it looks like there are underscores in the datafeed IDs so we can't use our normal x_y syntax
        match_object = re.match(r"^([^-]*)-([^-]*)\.json$", datafeed_file)
        file_path = os.path.join(datafeed_dir, datafeed_file)
        if match_object:
            datafeed_id = match_object.group(1)
            timestamp = match_object.group(2)
            try:
                with open(file_path, 'rb') as file_to_read:
                    body = file_to_read.read()
                    body = body.decode("utf-8")
                    validate_string_as_json(body)
                    yield file_path, datafeed_id, timestamp, body
            except (json.JSONDecodeError, UnicodeDecodeError) as error:
                LOGGER.error("Failed to load datafeed json file \"{}\". Error: {}".format(file_path, str(error)))
                remove_datafeed_file(file_path)
            except OSError as error:
                # OSErrors can happen here due to insufficient permissions or the file is no longer there.
                # In both situations there is no point attempting to remove the file so just log the error.
                LOGGER.error("Failed to read datafeed json file \"{}\". Error: {}".format(file_path, str(error)))
        else:
            LOGGER.warning("Malformed datafeed file: %s", datafeed_file)
            remove_datafeed_file(file_path)
