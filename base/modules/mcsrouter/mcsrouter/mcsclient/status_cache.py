#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.
"""
status_cache Module
"""



import re
import time
import logging

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
        self.__m_adapter_status_cache = {}

    def has_status_changed_and_record(self, app_id, adapter_status_xml):
        """
        Checks if an adapter has changed status, records new status if changed

        @return True if status changed

        """
        assert isinstance(adapter_status_xml, str)
        now = time.time()

        adapter_status_xml = adapter_status_xml.replace('&quot;', '"')
        adapter_status_xml = TIMESTAMP_RE.sub('', adapter_status_xml)

        previous_time, previous_status = self.__m_adapter_status_cache.get(
            app_id, (0, ""))

        if adapter_status_xml == previous_status and (
                now - previous_time) < MAX_STATUS_DELAY:
            LOGGER.debug("Adapter %s hasn't changed status", app_id)
            return False

        LOGGER.debug(
            "Adapter %s changed status: %s",
            app_id,
            adapter_status_xml)
        self.__m_adapter_status_cache[app_id] = (now, adapter_status_xml)

        return True

    def clear_cache(self):
        """
        clear_cache
        """
        self.__m_adapter_status_cache = {}
