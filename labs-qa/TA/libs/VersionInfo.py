#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2024 Sophos Plc, Oxford, England.
# All rights reserved.


import json
import re
from robot.api import logger


THREAT_DETECTOR_LOG = "/opt/sophos-spl/plugins/av/chroot/log/sophos_threat_detector.log"
VERSIONS_LINE_RE = r"SUSI Libraries loaded: (\{.*?\})$"


def get_vdb_and_engine_versions(logfile = THREAT_DETECTOR_LOG):
    """
    Scan the threat detector log for lines that contain version numbers.

    :param logfile: Name of the log file to scan.
    :return: A dict with keys 'Engine' and 'VDB', and version numbers
             (or "UNKNOWN") as values.
    """
    versions = {'Engine': "UNKNOWN", 'VDB': "UNKNOWN"}

    try:
        with open(logfile, 'r') as log:
            for line in log:
                m = re.search(VERSIONS_LINE_RE, line)
                if m:
                    maybe_json = m.group(1)
                    try:
                        json_struct = json.loads(maybe_json)
                    except json.decoder.JSONDecodeError:
                        continue
                        
                    if json_struct:
                        if 'savi' in json_struct:
                            versions['Engine'] = json_struct['savi']
                        if 'vdb' in json_struct:
                            versions['VDB'] = json_struct['vdb']
    except Exception:
        logger.warn("Error: could not read " + logfile)
        
    return versions