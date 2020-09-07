#!/usr/bin/env python3
# Copyright 2020 Sophos Plc, Oxford, England.

from . import sec_obfuscation
from . import path_manager
from .get_ids import get_gid, get_uid

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
