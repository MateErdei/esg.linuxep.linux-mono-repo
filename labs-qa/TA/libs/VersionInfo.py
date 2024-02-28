#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2024 Sophos Plc, Oxford, England.
# All rights reserved.


import json
import re
from robot.api import logger


THREAT_DETECTOR_LOG = "/opt/sophos-spl/plugins/av/chroot/log/sophos_threat_detector.log"
VERSIONS_LINE_RE = r"SUSI Libraries loaded: (\{.*?\})$"


def get_susi_configuration(versions_to_return = None):
    """
    Scan the threat detector log for lines that contain version numbers.

    :param :obj:`list` versions_to_return: A list of version data types. The
                       function will return only the data types specified in
                       this list. Unnown data types are ignored.
                       if the list is empty, *all* known data types will be
                       returned.
    :return: A dict with version data types as keys and the corresponding
             version numbers (or "UNKNOWN") as values.
    """
    # Keys: known data types
    # Values: corresponding attributes in the "SUSI Libraries loaded:" JSON
    #         data structure, or None    
    attribute_map = {
        'SFS_Data': "vdb",
        'SFS_Engine': "savi",
        'ML_Data': "mldata",
        # 'ML_Telemetry_Data': None,
        'Localrep_Data': "lrdata"
    }
    versions = {}

    for data_type in versions_to_return:
        if data_type in attribute_map:
            versions[data_type] = "UNKNOWN"

    if not versions:
        for key in attribute_map.keys():
            versions[key] = "UNKNOWN"

    try:
        with open(THREAT_DETECTOR_LOG, 'r') as log:
            for line in log:
                m = re.search(VERSIONS_LINE_RE, line)
                if m:
                    maybe_json = m.group(1)
                    try:
                        json_struct = json.loads(maybe_json)
                    except json.decoder.JSONDecodeError:
                        json_struct = None
                        continue

                    if json_struct:
                        for data_type in versions:
                            json_attr = attribute_map[data_type]
                            if json_attr in json_struct:
                                versions[data_type] = json_struct[json_attr]
    except Exception:
        logger.warn("Error: could not read the threat detector log!")

    return versions