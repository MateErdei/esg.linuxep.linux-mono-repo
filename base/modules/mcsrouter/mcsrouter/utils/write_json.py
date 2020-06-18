#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import sec_obfuscation
from . import path_manager

import logging
import json
import os
LOGGER = logging.getLogger(__name__)

def write_current_proxy_info(proxy):
    folder = path_manager.sophos_etc_dir()
    filepath = os.path.join(folder, "current_proxy")
    proxy_info = {}

    if proxy.address():
        proxy_info['address'] = proxy.address()

        if proxy.relay_id():
            proxy_info['relay_id'] = proxy.relay_id()

        elif proxy.username():

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