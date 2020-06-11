#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import path_manager

import logging
import json
import os
LOGGER = logging.getLogger(__name__)

def write_current_proxy_info(proxy):
    folder = path_manager.etc_dir()
    filepath = os.path.join(folder, "current_proxy")
    proxy_info = {}
    proxy_info['relay_id'] = proxy.relay_id()
    proxy_info['host'] = proxy.host()
    proxy_info['port'] = proxy.port()

    with open(filepath, 'w') as outfile:
        json.dump(proxy_info, outfile)