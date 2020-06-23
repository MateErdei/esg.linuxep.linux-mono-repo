# Copyright 2020 Sophos Plc, Oxford, England.
"""
livequery_adapter Module
"""

import datetime
import logging
import os

import mcsrouter.adapters.generic_adapter as generic_adapter
import mcsrouter.adapters.adapter_base
import mcsrouter.utils.atomic_write
import mcsrouter.utils.timestamp
import mcsrouter.utils.utf8_write
import mcsrouter.utils.xml_helper as xml_helper

LOGGER = logging.getLogger(__name__)

class LiveQueryAdapter(generic_adapter.GenericAdapter):
    """
    LiveQueryAdapter class override _get_action_name to handle processing of LiveQuery request
    LiveQuery request require that their file written to the actions folder has its name as:
     '<AppId=LiveQuery>_<correlation-id=id>_request_<timestamp=creationTime>_<ttl=ttl+creationTime>.json'
    """
    def __init__(self, app_id, install_dir):
        super().__init__(app_id, install_dir)

    def _get_action_name(self, command):
        app_id = self.get_app_id()
        LOGGER.debug("{} received an action".format(app_id))

        correlation_id = command.get("id")
        try:
            timestamp = command.get("creationTime")
        except KeyError as ex:
            LOGGER.error("Could not get creationTime, using the timestamp. Exception: {}".format(ex))
            timestamp = mcsrouter.utils.timestamp.timestamp()
        ttl = self._convert_ttl_to_epoch_time(timestamp, command.get("ttl"))
        order_tag = datetime.datetime.now().strftime("%Y%m%d%H%M%S%f")
        return f"{order_tag}_{app_id}_{correlation_id}_request_{timestamp}_{ttl}.json"
