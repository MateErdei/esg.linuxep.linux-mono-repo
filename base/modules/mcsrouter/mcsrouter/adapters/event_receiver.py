#!/usr/bin/env python
"""
event_receiver Module
"""



import os
import re

import logging
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
            except Exception as ex:
                LOGGER.error( "Failed verification of xml as it has script. Error: {}".format(str(ex)))
                continue
            yield (app_id, time, body)
        else:
            LOGGER.warning("Malformed event file: %s", event_file)

        os.remove(file_path)
