#!/usr/bin/env python
"""
EventReceiver Module
"""

from __future__ import print_function, division, unicode_literals

import os
import re

import logging
import mcsrouter.utils.PathManager as PathManager
import mcsrouter.utils.XmlHelper as XmlHelper

LOGGER = logging.getLogger(__name__)


class EventReceiver(object):
    """
    EventReceiver
    """

    def __init__(self, install_dir):
        """
        __init__
        """
        if install_dir is not None:
            PathManager.INST = install_dir

    def receive(self):
        """
        Async receive call
        """

        event_dir = os.path.join(PathManager.event_dir())
        for event_file in os.listdir(event_dir):
            match_object = re.match(r"([A-Z]*)_event-(.*).xml", event_file)
            file_path = os.path.join(event_dir, event_file)
            if match_object:
                app_id = match_object.group(1)
                time = os.path.getmtime(file_path)
                body = XmlHelper.get_xml_file_content_with_escaped_non_ascii_code(
                    file_path)
                yield (app_id, time, body)
            else:
                LOGGER.warning("Malformed event file: %s", event_file)

            os.remove(file_path)
