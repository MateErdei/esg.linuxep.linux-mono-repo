#!/usr/bin/env python3
"""
event_receiver Module
"""



import logging
import os
import re

import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper as xml_helper

LOGGER = logging.getLogger(__name__)


def receive():
    """
    Async receive call
    """

    event_dir = os.path.join(path_manager.event_dir())
    for event_file in os.listdir(event_dir):
        match_object = re.match(r"([A-Z]*)_event-(.*).xml", event_file)
        file_path = os.path.join(event_dir, event_file)
        if match_object:
            app_id = match_object.group(1)
            time = os.path.getmtime(file_path)
            body = xml_helper.get_escaped_non_ascii_content(
                file_path)
            try:
                xml_helper.check_xml_has_no_script_tags(body)
            except xml_helper.XMLException as ex:
                LOGGER.error("Failed verification of XML as it contains script tags. Error: {}".format(str(ex)))
                continue
            yield (app_id, time, body)
        else:
            LOGGER.warning("Malformed event file: %s", event_file)

        os.remove(file_path)
