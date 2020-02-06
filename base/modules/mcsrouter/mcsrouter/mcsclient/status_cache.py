#!/usr/bin/env python3
# Copyright (C) 2017-2019 Sophos Plc, Oxford, England.
# All rights reserved.
"""
status_cache Module
"""



import logging
import os
import re
import time
import hashlib
import json


LOGGER = logging.getLogger(__name__)

MAX_STATUS_DELAY = 7 * 24 * 60 * 60

TIMESTAMP_RE = re.compile(
    r'&lt;creationTime&gt;[^&]+&lt;/creationTime&gt;|timestamp=\"[^\"\']*\"|timestamp=\'[^\"\']*\'')


class StatusCache:
    """
    StatusCache class
    """

    def __init__(self):
        """
        __init__
        """

    def _remove_timestamp_from_status(self, status_xml):
        status_xml = status_xml.replace('&quot;', '"')
        status_xml = TIMESTAMP_RE.sub('', status_xml)
        return status_xml

    def _hash_status(self, status_xml):
        hash_object = hashlib.md5(status_xml.encode())
        return hash_object.hexdigest()


    def has_status_changed_and_record(self, app_id, adapter_status_xml, status_cache_dir):
        """
        Checks if an adapter has changed status, records new status if changed

        @return True if status changed

        """

        adapter_status_cache = {}

        assert isinstance(adapter_status_xml, str)
        now = time.time()

        adapter_status_xml = self._remove_timestamp_from_status(adapter_status_xml)

        hashed_adapter_status_xml = self._hash_status(adapter_status_xml)

        cached_status_file_path = os.path.join(status_cache_dir, 'status_cache.json')

        cached_status_hash = ""
        cached_status_file_time_stamp = 0

        if os.path.isfile(cached_status_file_path):
            with open(cached_status_file_path, 'r') as cached_status_json_infile:
                adapter_status_cache = json.load(cached_status_json_infile)

                app_id_data = adapter_status_cache.get(app_id)

                if app_id_data:
                    cached_status_file_time_stamp = app_id_data['timestamp']
                    cached_status_hash = app_id_data['status_hash']

        if hashed_adapter_status_xml == cached_status_hash and (
                now - cached_status_file_time_stamp) < MAX_STATUS_DELAY:
            LOGGER.debug("Adapter %s hasn't changed status", app_id)
            return False

        LOGGER.debug(
            "Adapter %s changed status: %s",
            app_id,
            adapter_status_xml)
        adapter_status_cache[app_id] = {'timestamp': now, 'status_hash': hashed_adapter_status_xml}

        with open(cached_status_file_path, 'w') as cached_status_json_outfile:
            json.dump(adapter_status_cache, cached_status_json_outfile)

        return True

    def clear_cache(self, status_cache_dir):
        """
        clear_cache
        """
        full_path = os.path.join(status_cache_dir, 'status_cache.json')

        os.remove(full_path)
