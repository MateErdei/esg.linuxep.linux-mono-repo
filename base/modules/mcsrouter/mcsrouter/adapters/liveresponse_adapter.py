# Copyright 2020 Sophos Plc, Oxford, England.
"""
liveresponse_adapter Module
"""

import datetime
import logging
import os

import mcsrouter.adapters.generic_adapter as generic_adapter
import mcsrouter.utils.timestamp

LOGGER = logging.getLogger(__name__)

class LiveResponseAdapter(generic_adapter.GenericAdapter):
    """
    LiveResponseAdapter class override _get_action_name to handle processing of LiveResponse action
    LiveResponse action require that their file written to the actions folder has its name as:
     '<AppId=LiveQuery>_action_<timestamp=creationTime>_<timeToLive>.json'
    """
    def __init__(self, app_id, install_dir):
        super().__init__(app_id, install_dir)

    def _get_action_name(self, command):
        try:
            timestamp = command.get("creationTime")
        except KeyError:
            timestamp = mcsrouter.utils.timestamp.timestamp()
        ttl = self._convert_ttl_to_epoch_time(timestamp, command.get("ttl"))

        order_tag = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
        return f"{order_tag}_{self.get_app_id()}_action_{timestamp}_{ttl}.xml"
