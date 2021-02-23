#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import sec_obfuscation
from . import path_manager
from .get_ids import get_gid, get_uid

import logging
import json
import os
import time
LOGGER = logging.getLogger(__name__)

def write_current_proxy_info(proxy):
    filepath = path_manager.mcs_current_proxy()
    proxy_info = {}

    if proxy.host():
        proxy_info['proxy'] = proxy.host()
        if proxy.port():
            proxy_info['proxy'] = proxy.host() + ':' + str(proxy.port())
        if proxy.relay_id():
            proxy_info['relay_id'] = proxy.relay_id()

        elif proxy.username() and proxy.password():
            proxycredentials = proxy.username() + ":" + proxy.password()
            cred_encoded = proxycredentials.encode('utf-8')
            obfuscated = sec_obfuscation.obfuscate(
                sec_obfuscation.ALGO_AES256,
                cred_encoded)
            proxy_info['credentials'] = obfuscated.decode('utf-8')
    else:
        try:
            if os.path.exists(filepath):
                os.remove(filepath)
        except Exception as e:
            LOGGER.warning("Failed to clean up current proxy file with error {}".format(e))

    with open(filepath, 'w') as outfile:
        json.dump(proxy_info, outfile)
    os.chmod(filepath, 0o640)

    # If registering with a proxy/mr this will be run as root so need to change ownership
    if os.getuid() == 0:
        os.chown(filepath, get_uid("sophos-spl-local"), get_gid("sophos-spl-group"))

def write_mcs_flags(info):
    flag_file_path = path_manager.mcs_flags_file()
    with open(flag_file_path, 'w') as outfile:
        outfile.write(info)

def read_datafeed_tracker():
    tracker_filepath = path_manager.datafeed_tracker()
    tracker_json = {}
    if os.path.exists(tracker_filepath):
        try:
            with open(tracker_filepath, 'r') as file:
                contents = file.read()
                tracker_json = json.loads(contents)

                #check size is valid
                try:
                    int(tracker_json['size'])
                except (ValueError, TypeError):
                    LOGGER.warning(f"size {tracker_json['size']} in file: {tracker_filepath} not a valid int")
                    tracker_json['size'] = 0
                except KeyError:
                    LOGGER.warning(f"size field not found in file: {tracker_filepath}")
                    tracker_json['size'] = 0

                #check epoch time is valid
                try:
                    int(tracker_json["time_sent"])
                except (ValueError, TypeError):
                    LOGGER.warning(f"Epoch time {tracker_json['time_sent']} in file: {tracker_filepath} not a valid int, resetting to current time")
                    tracker_json["time_sent"] = int(time.time())
                except KeyError:
                    LOGGER.warning(f"time_sent field not found in file: {tracker_filepath}")
                    tracker_json["time_sent"] = int(time.time())

        except (PermissionError, json.JSONDecodeError) as e:
            LOGGER.warning("Unable to read {} with error: {}".format(tracker_filepath, e))
            tracker_json['size'] = 0
            tracker_json['time_sent'] = int(time.time())
    else:
        tracker_json['size'] = 0
        tracker_json['time_sent'] = int(time.time())
    return tracker_json

def update_datafeed_tracker(datafeed_info, size):
    tracker_filepath = path_manager.datafeed_tracker()

    datafeed_info['size'] += size
    current_time = int(time.time())

    time_since_last_sent = current_time - datafeed_info['time_sent']
    if time_since_last_sent >= 24*60*60:

        data_size_in_kB = datafeed_info['size']/1000
        elasped_time_hours = round(time_since_last_sent/3600, 1)
        LOGGER.info(f"In the past {elasped_time_hours}h we have sent {data_size_in_kB}kB of scheduled query data to Central")

        datafeed_info['time_sent'] = current_time
        datafeed_info['size'] = 0

    with open(tracker_filepath, 'w') as outfile:
        json.dump(datafeed_info, outfile)
    os.chmod(tracker_filepath, 0o640)

def update_datafeed_size(size):
    datafeed_info = read_datafeed_tracker()
    update_datafeed_tracker(datafeed_info, size)