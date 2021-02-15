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
    filepath = path_manager.datafeed_tracker()
    file_data = {}
    if os.path.exists(filepath):
        try:
            with open(filepath, 'r') as outfile:
                contents = outfile.read()
                file_data = json.loads(contents)
        except PermissionError as e:
            LOGGER.warning("Unable to read {} with error: ".format(filepath, e))
            file_data['size'] = 0
            file_data['time_sent'] = int(time.time())
    else:
        file_data['size'] = 0
        file_data['time_sent'] = int(time.time())
    return file_data

def update_datafeed_tracker(datafeed_info, size):
    filepath = path_manager.datafeed_tracker()

    try:
        datafeed_info['size'] += size
    except ValueError:
        LOGGER.warning(f"size {datafeed_info['size']} in file: {filepath} not a valid int")
        datafeed_info['size'] = size

    current_time = int(time.time())
    if (current_time - datafeed_info["time_sent"]) > 24*60*60:
        time_string = time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime(datafeed_info['time_sent']))
        data_size_in_kB = datafeed_info['size']/1000
        LOGGER.info(f"Sent {data_size_in_kB}kB of datafeed to Central since {time_string}")
        datafeed_info['time_sent'] = current_time
        datafeed_info['size'] = 0

    with open(filepath, 'w') as outfile:
        json.dump(datafeed_info, outfile)
    os.chmod(filepath, 0o640)

def update_datafeed_size(size):
    datafeed_info = read_datafeed_tracker()
    update_datafeed_tracker(datafeed_info, size)