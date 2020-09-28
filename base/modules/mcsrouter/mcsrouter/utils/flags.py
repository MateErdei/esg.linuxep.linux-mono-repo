#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import path_manager
from . import atomic_write

import json
import logging
import os
import stat

LOGGER = logging.getLogger(__name__)

def write_combined_flags_file(body):
    """
    write_combined_flags_file
    :param body: Contents of file to be written atomically to flags.json
    """
    try:
        path_tmp = os.path.join(path_manager.policy_temp_dir(), "flags.json")
        target_path = path_manager.combined_flags_file()
        LOGGER.debug("Writing combined flags to {}".format(target_path))
        body = json.dumps(body)
        atomic_write.atomic_write(target_path, path_tmp, body)
        # Make sure that flags are group readable so that Management Agent (or plugins) can read the file.
        os.chmod(target_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP )
        LOGGER.info("Flags combined and written to disk")
    except TypeError as error:
        LOGGER.error("Failed to encode json: {}".format(error))
    except OSError as error:
        # OSErrors can happen here due to insufficient permissions on an existing file.
        # There is no point attempting to remove the file so just log the error.
        LOGGER.error("Failed to change permission on json file \"{}\". Error: {}".format(target_path, str(error)))

def read_flags_file(file_path):
    """
    read_flags_file
    :param file_path: a flags json file
    :return: file contents as a dictionary.
    Returns empty dict if fails to retrieve contents
    """
    LOGGER.debug("Reading content of {}".format(file_path))
    try:
        with open(file_path, 'rb') as file_to_read:
            body = file_to_read.read()
            body = body.decode("utf-8")
            return json.loads(body)
    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        LOGGER.error("Failed to load json file \"{}\". Error: {}".format(file_path, str(error)))
    except OSError as error:
        # OSErrors can happen here due to insufficient permissions or the file is no longer there.
        # In both situations there is no point attempting to remove the file so just log the error.
        LOGGER.error("Failed to read json file \"{}\". Error: {}".format(file_path, str(error)))
    return {}

def create_comparable_dicts(mcs_dict, warehouse_dict):
    """
    create_comparable_dicts
    :param mcs_dict: mcs flags dictionary
    :param second_dict: a dictionary
    :return: Two dictionaries where all keys from input dicts are present.
    Default values to false if key not present in original dict
    """
    set_of_flags = []
    for i in mcs_dict:
        set_of_flags.append(i)
    for i in warehouse_dict:
        set_of_flags.append(i)
    set_of_flags = list(dict.fromkeys(set_of_flags))
    all_flags_mcs = {}
    all_flags_warehouse = {}
    for i in set_of_flags:
        try:
            all_flags_mcs[i] = mcs_dict[i]
        except KeyError:
            LOGGER.debug("Flag {} missing from MCS flags therefore defaulting to false".format(i))
            all_flags_mcs[i] = "false"
        try:
            all_flags_warehouse[i] = warehouse_dict[i]
        except KeyError:
            LOGGER.debug("Flag {} missing from Warehouse flags therefore defaulting to false".format(i))
            all_flags_warehouse[i] = "false"
    return all_flags_mcs, all_flags_warehouse

def combine_flags(warehouse_flags, mcs_flags):
    """
    combine_flags
    :param warehouse_flags: dictionary containing all of the warehouse flags
    :param mcs_flags: dictionary containing all of the mcs flags
    :return: dictionary containing a combination of all flags
    Follows the AND logic table but where force overrides to true in all situations
    """
    combined_flags = {}
    for i in warehouse_flags:
        if warehouse_flags[i] == "force" or \
                (warehouse_flags[i] == "true" and mcs_flags[i] == "true"):
            combined_flags[i] = "true"
        else:
            combined_flags[i] = "false"
    return combined_flags

def combine_flags_files():
    """
    combine_flags_files
    """
    LOGGER.info("Combining MCS and Warehouse flags")
    mcs_flags = read_flags_file(path_manager.mcs_flags_file())
    LOGGER.debug("MCS flags: {}".format(mcs_flags))
    warehouse_flags = read_flags_file(path_manager.warehouse_flags_file())
    LOGGER.debug("Warehouse flags: {}".format(warehouse_flags))
    mcs_flags, warehouse_flags = create_comparable_dicts(mcs_flags, warehouse_flags)
    combined_flags = combine_flags(warehouse_flags, mcs_flags)
    LOGGER.debug("Combined flags: {}".format(combined_flags))
    write_combined_flags_file(combined_flags)
