#!/usr/bin/env python
# Copyright (C) 2017 Sophos Plc, Oxford, England.
# All rights reserved.

from __future__ import print_function,division,unicode_literals

import re
import time
import logging

logger = logging.getLogger(__name__)

MAX_STATUS_DELAY = 7 * 24 * 60 * 60

TIMESTAMP_RE = re.compile(r'&lt;creationTime&gt;[^&]+&lt;/creationTime&gt;|timestamp=\"[^\"\']*\"|timestamp=\'[^\"\']*\'')

class StatusCache(object):
    def __init__(self):
        self.__m_adapterStatusCache = {}

    def hasStatusChangedAndRecord(self, appID, adapterStatusXML):
        """
        Checks if an adapter has changed status, records new status if changed

        @return True if status changed

        """
        now = time.time()

        adapterStatusXML = adapterStatusXML.replace('&quot;','"')
        adapterStatusXML = TIMESTAMP_RE.sub('',adapterStatusXML)

        previousTime,previousStatus = self.__m_adapterStatusCache.get(appID,(0,""))

        if adapterStatusXML == previousStatus and (now - previousTime) < MAX_STATUS_DELAY:
            logger.debug("Adapter %s hasn't changed status",appID)
            return False

        logger.debug("Adapter %s changed status: %s",appID,adapterStatusXML)
        self.__m_adapterStatusCache[appID] = (now,adapterStatusXML)

        return True

    def clearCache(self):
        self.__m_adapterStatusCache = {}
