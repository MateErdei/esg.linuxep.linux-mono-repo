#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import sec_obfuscation
from . import path_manager

import logging
import json
import os
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
            os.remove(filepath)
        except Exception as e:
            LOGGER.warning("Failed to clean up current proxy file with error {}".format(e))



    with open(filepath, 'w') as outfile:
        json.dump(proxy_info, outfile)
